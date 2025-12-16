#include "Http2Client.h"
#include "Http2ClientResponse.h"
#include "Http2ClientImpl.h"
#include <sstream>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace astra::http2 {

// ============================================================================
// ClientResponse Implementation
// ============================================================================

ClientResponse::ClientResponse() : m_impl(std::make_shared<Impl>(0)) {}
ClientResponse::ClientResponse(std::shared_ptr<Impl> impl) : m_impl(impl) {}
ClientResponse::~ClientResponse() = default;

int ClientResponse::status_code() const { return m_impl->status_code; }
const std::string& ClientResponse::body() const { return m_impl->body; }

std::string ClientResponse::header(const std::string& name) const {
    auto it = m_impl->headers.find(name);
    if (it != m_impl->headers.end()) {
        return it->second;
    }
    return "";
}

const std::map<std::string, std::string>& ClientResponse::headers() const {
    return m_impl->headers;
}

// ============================================================================
// Client Implementation
// ============================================================================

Client::Client(const ClientConfig& config) : m_impl(std::make_unique<ClientImpl>(config)) {}
Client::~Client() = default;

void Client::get(const std::string& path, ResponseHandler handler) {
    submit("GET", path, "", {}, handler);
}

void Client::post(const std::string& path, const std::string& body, ResponseHandler handler) {
    submit("POST", path, body, {}, handler);
}

void Client::submit(const std::string& method, const std::string& path, 
                    const std::string& body, 
                    const std::map<std::string, std::string>& headers,
                    ResponseHandler handler) {
    m_impl->submit(method, path, body, headers, handler);
}

bool Client::is_connected() const {
    return m_impl->is_connected();
}

// ============================================================================
// ClientImpl Implementation
// ============================================================================

ClientImpl::ClientImpl(const ClientConfig& config) : m_config(config) {
    // Lazy connection: only start io_service, don't connect yet
    start_io_service();
}

ClientImpl::~ClientImpl() {
    stop_io_service();
}

void ClientImpl::start_io_service() {
    m_work = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
        boost::asio::make_work_guard(m_io_context));
    m_io_thread = std::thread([this]() {
        try {
            m_io_context.run();
        } catch (const std::exception& e) {
            obs::error(std::string("IO Service error: ") + e.what());
        }
    });
}

void ClientImpl::stop_io_service() {
    // Only attempt graceful shutdown if session was successfully connected.
    // If connection failed, the nghttp2 session handle inside m_session is null
    // and calling shutdown() will SEGFAULT.
    if (m_session && m_state.load(std::memory_order_acquire) == ConnectionState::CONNECTED) {
        m_session->shutdown();
    }
    m_session.reset();
    
    m_work.reset();
    m_io_context.stop();
    
    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
}


void ClientImpl::ensure_connected() {
    // Fast path: already connected
    if (m_state.load(std::memory_order_acquire) == ConnectionState::CONNECTED) {
        return;
    }
    
    // Slow path: need to connect (with mutex to prevent multiple connect attempts)
    std::lock_guard<std::mutex> lock(m_connect_mutex);
    
    // Double-check after acquiring lock
    ConnectionState current = m_state.load(std::memory_order_acquire);
    if (current == ConnectionState::CONNECTED || current == ConnectionState::CONNECTING) {
        return;
    }
    
    // Reset from FAILED state if needed
    m_state.store(ConnectionState::CONNECTING, std::memory_order_release);
    connect();
}

void ClientImpl::connect() {
    try {
        boost::system::error_code ec;
        std::string port_str = std::to_string(m_config.port());
        
        m_session = std::make_unique<nghttp2::asio_http2::client::session>(
            m_io_context, m_config.host(), port_str);

        m_session->on_connect([this](boost::asio::ip::tcp::resolver::results_type::iterator endpoint_it) {
            m_state.store(ConnectionState::CONNECTED, std::memory_order_release);
            obs::info("Connected to " + m_config.host() + ":" + std::to_string(m_config.port()));
            // Flush any requests that were queued while connecting
            flush_pending_requests();
        });

        m_session->on_error([this](const boost::system::error_code& ec) {
            m_state.store(ConnectionState::FAILED, std::memory_order_release);
            obs::error("Connection error: " + ec.message());
            // Fail all pending requests - must hold mutex since submit() also accesses the queue
            std::lock_guard<std::mutex> lock(m_connect_mutex);
            while (!m_pending_requests.empty()) {
                auto& req = m_pending_requests.front();
                req.handler(ClientResponse(), Error{1, "Connection failed: " + ec.message()});
                m_pending_requests.pop();
            }
        });

    } catch (const std::exception& e) {
        m_state.store(ConnectionState::FAILED, std::memory_order_release);
        obs::error("Failed to create session: " + std::string(e.what()));
    }
}

bool ClientImpl::is_connected() const {
    return m_state.load(std::memory_order_acquire) == ConnectionState::CONNECTED;
}

ConnectionState ClientImpl::state() const {
    return m_state.load(std::memory_order_acquire);
}

void ClientImpl::submit(const std::string& method, const std::string& path, 
                        const std::string& body, 
                        const std::map<std::string, std::string>& headers,
                        ResponseHandler handler) {
    
    ConnectionState current = m_state.load(std::memory_order_acquire);
    
    if (current == ConnectionState::FAILED) {
        obs::debug("submit: returning error - state is FAILED");
        handler(ClientResponse(), Error{1, "Connection failed"});
        return;
    }
    
    if (current == ConnectionState::CONNECTED) {
        // Already connected - submit directly
        do_submit(method, path, body, headers, handler);
        return;
    }
    
    // Not connected yet - queue the request
    {
        std::lock_guard<std::mutex> lock(m_connect_mutex);
        m_pending_requests.push({method, path, body, headers, handler});
    }
    
    // Start connecting if not already
    ensure_connected();
}

void ClientImpl::flush_pending_requests() {
    // Called from on_connect callback - on io_context thread
    std::lock_guard<std::mutex> lock(m_connect_mutex);
    while (!m_pending_requests.empty()) {
        auto req = std::move(m_pending_requests.front());
        m_pending_requests.pop();
        do_submit(req.method, req.path, req.body, req.headers, req.handler);
    }
}

void ClientImpl::do_submit(const std::string& method, const std::string& path,
                           const std::string& body,
                           const std::map<std::string, std::string>& headers,
                           ResponseHandler handler) {
    boost::asio::post(m_io_context, [this, method, path, body, headers, handler]() {
        if (m_state.load(std::memory_order_acquire) != ConnectionState::CONNECTED) {
            obs::debug("do_submit: returning error - not connected");
            handler(ClientResponse(), Error{1, "Not connected"});
            return;
        }

        boost::system::error_code ec;
        
        // Convert headers
        nghttp2::asio_http2::header_map ng_headers;
        for (const auto& kv : headers) {
            ng_headers.emplace(kv.first, nghttp2::asio_http2::header_value{kv.second, false});
        }

        // Build full URI - nghttp2 asio submit requires full URI, not just path
        std::string uri = "http://" + m_config.host() + ":" + std::to_string(m_config.port()) + path;
        
        // Debug: check session validity before submit
        if (!m_session) {
            obs::debug("do_submit: m_session is nullptr!");
            handler(ClientResponse(), Error{1, "Session is null"});
            return;
        }
        
        auto req = m_session->submit(ec, method, uri, body, ng_headers);

        if (ec) {
            obs::debug("do_submit: submit failed - ec=" + std::to_string(ec.value()) + " msg=" + ec.message());
            
            // Error code 2 = EOF (connection closed by peer)
            // Mark as disconnected so next request triggers reconnection
            if (ec.value() == 2) {
                obs::info("Connection closed by peer - will reconnect on next request");
                m_state.store(ConnectionState::DISCONNECTED, std::memory_order_release);
                m_session.reset();  // Clear the dead session
            }
            
            handler(ClientResponse(), Error{ec.value(), "Submit failed: " + ec.message()});
            return;
        }
        
        if (!req) {
            obs::debug("do_submit: submit returned nullptr but no error code!");
            handler(ClientResponse(), Error{1, "Submit returned nullptr"});
            return;
        }

        // Request Timeout Timer
        uint32_t timeout_ms = m_config.request_timeout_ms() > 0 ? m_config.request_timeout_ms() : 10000;
        auto timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
        timer->expires_from_now(boost::posix_time::milliseconds(timeout_ms));
        
        // Shared state to coordinate between timeout and response
        struct RequestState {
            bool completed = false;
            std::shared_ptr<ClientResponse::Impl> response_impl = std::make_shared<ClientResponse::Impl>(0);
        };
        auto state = std::make_shared<RequestState>();

        timer->async_wait([req, state, handler](const boost::system::error_code& ec) {
            if (!ec && !state->completed) {
                state->completed = true;
                req->cancel(NGHTTP2_CANCEL);
                handler(ClientResponse(), Error{2, "Request timeout"});
            }
        });

        req->on_response([state, timer, handler](const nghttp2::asio_http2::client::response& res) {
            state->response_impl->status_code = res.status_code();
            
            for (const auto& kv : res.header()) {
                state->response_impl->headers[kv.first] = kv.second.value;
            }

            res.on_data([state](const uint8_t* data, std::size_t len) {
                if (len > 0) {
                    state->response_impl->body.append(reinterpret_cast<const char*>(data), len);
                }
            });
        });

        req->on_close([state, timer, handler](uint32_t error_code) {
            if (state->completed) return;
            
            timer->cancel();
            state->completed = true;

            if (error_code != 0) {
                obs::debug("on_close: Stream closed with error code " + std::to_string(error_code));
                handler(ClientResponse(), Error{(int)error_code, "Stream closed with error"});
            } else {
                handler(ClientResponse(state->response_impl), Error{});
            }
        });
    });
}

} // namespace astra::http2


