#include "Http2Client.h"
#include "Http2ClientImpl.h"
#include <sstream>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace http2client {

// ============================================================================
// Response Implementation
// ============================================================================

Response::Response() : m_impl(std::make_shared<Impl>(0)) {}
Response::Response(std::shared_ptr<Impl> impl) : m_impl(impl) {}
Response::~Response() = default;

int Response::status_code() const { return m_impl->status_code; }
const std::string& Response::body() const { return m_impl->body; }

std::string Response::header(const std::string& name) const {
    auto it = m_impl->headers.find(name);
    if (it != m_impl->headers.end()) {
        return it->second;
    }
    return "";
}

const std::map<std::string, std::string>& Response::headers() const {
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
    start_io_service();
    connect();
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
    if (m_session) {
        m_session->shutdown();
        m_session.reset();
    }
    
    m_work.reset();
    m_io_context.stop();
    
    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
}

void ClientImpl::connect() {
    try {
        boost::system::error_code ec;
        m_session = std::make_unique<nghttp2::asio_http2::client::session>(
            m_io_context, m_config.host, m_config.port);

        m_session->on_connect([this](boost::asio::ip::tcp::resolver::results_type::iterator endpoint_it) {
            m_connected = true;
            obs::info("Connected to " + m_config.host + ":" + m_config.port);
        });

        m_session->on_error([this](const boost::system::error_code& ec) {
            m_connected = false;
            obs::error("Connection error: " + ec.message());
            // Reconnect logic could go here, but for now we rely on retries or manual restart
        });

    } catch (const std::exception& e) {
        obs::error("Failed to create session: " + std::string(e.what()));
    }
}

bool ClientImpl::is_connected() const {
    return m_connected;
}

void ClientImpl::submit(const std::string& method, const std::string& path, 
                          const std::string& body, 
                          const std::map<std::string, std::string>& headers,
                          ResponseHandler handler) {
    
    // Ensure we run on the IO thread
    boost::asio::post(m_io_context, [this, method, path, body, headers, handler]() {
        // Initial submission with max retries
        schedule_retry(method, path, body, headers, handler, m_config.max_retries, Error{});
    });
}

void ClientImpl::schedule_retry(const std::string& method, const std::string& path, 
                                  const std::string& body, 
                                  const std::map<std::string, std::string>& headers,
                                  ResponseHandler handler,
                                  int retries_remaining,
                                  const Error& last_error) {

    // Check connection
    if (!m_connected) {
        if (retries_remaining > 0) {
            // Wait and retry
            std::shared_ptr<boost::asio::deadline_timer> timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
            timer->expires_from_now(boost::posix_time::milliseconds(m_config.retry_interval.count()));
            timer->async_wait([this, method, path, body, headers, handler, retries_remaining, last_error, timer](const boost::system::error_code& ec) {
                if (!ec) {
                    // Try to reconnect if needed? 
                    // For now, just retry submit, which might fail again if still disconnected
                    // Ideally we should trigger a reconnect here if session is dead
                    if (!m_session) connect(); 
                    schedule_retry(method, path, body, headers, handler, retries_remaining - 1, Error{1, "Not connected"});
                }
            });
            return;
        } else {
            handler(Response(), Error{1, "Not connected: " + (last_error.message.empty() ? "Connection failed" : last_error.message)});
            return;
        }
    }

    boost::system::error_code ec;
    
    // Convert headers
    nghttp2::asio_http2::header_map ng_headers;
    for (const auto& kv : headers) {
        ng_headers.emplace(kv.first, nghttp2::asio_http2::header_value{kv.second, false});
    }

    auto req = m_session->submit(ec, method, path, body, ng_headers);

    if (ec) {
        if (retries_remaining > 0) {
             std::shared_ptr<boost::asio::deadline_timer> timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
            timer->expires_from_now(boost::posix_time::milliseconds(m_config.retry_interval.count()));
            timer->async_wait([this, method, path, body, headers, handler, retries_remaining, ec, timer](const boost::system::error_code&) {
                schedule_retry(method, path, body, headers, handler, retries_remaining - 1, Error{ec.value(), ec.message()});
            });
        } else {
            handler(Response(), Error{ec.value(), "Submit failed: " + ec.message()});
        }
        return;
    }

    // Request Timeout Timer
    std::shared_ptr<boost::asio::deadline_timer> timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
    timer->expires_from_now(boost::posix_time::milliseconds(m_config.request_timeout.count()));
    
    // Shared state to coordinate between timeout and response
    struct RequestState {
        bool completed = false;
        std::shared_ptr<Response::Impl> response_impl = std::make_shared<Response::Impl>(0);
    };
    auto state = std::make_shared<RequestState>();

    timer->async_wait([req, state, handler, this, method, path, body, headers, retries_remaining](const boost::system::error_code& ec) {
        if (!ec && !state->completed) {
            // Timeout occurred
            state->completed = true;
            req->cancel(NGHTTP2_CANCEL);
            
            // Retry on timeout? Usually yes for idempotent requests, but here we retry all per config
            if (retries_remaining > 0) {
                schedule_retry(method, path, body, headers, handler, retries_remaining - 1, Error{2, "Request timeout"});
            } else {
                handler(Response(), Error{2, "Request timeout"});
            }
        }
    });

    req->on_response([this, state, timer, handler, method, path, body, headers, retries_remaining](const nghttp2::asio_http2::client::response& res) {
        state->response_impl->status_code = res.status_code();
        
        // Copy headers
        for (const auto& kv : res.header()) {
            state->response_impl->headers[kv.first] = kv.second.value;
        }

        res.on_data([state](const uint8_t* data, std::size_t len) {
            if (len > 0) {
                state->response_impl->body.append(reinterpret_cast<const char*>(data), len);
            }
        });

    });

    req->on_close([this, state, timer, handler, method, path, body, headers, retries_remaining](uint32_t error_code) {
        if (state->completed) return; // Already handled (e.g. timeout)
        
        timer->cancel(); // Cancel timeout timer
        state->completed = true;

        if (error_code != 0) {
            // Network/Protocol error
            if (retries_remaining > 0) {
                schedule_retry(method, path, body, headers, handler, retries_remaining - 1, Error{(int)error_code, "Stream closed with error"});
            } else {
                handler(Response(), Error{(int)error_code, "Stream closed with error"});
            }
        } else {
            // Success (protocol wise)
            // Check for 5xx if retry_on_server_error is true
            int status = state->response_impl->status_code;
            if (m_config.retry_on_server_error && status >= 500 && status < 600 && retries_remaining > 0) {
                schedule_retry(method, path, body, headers, handler, retries_remaining - 1, Error{status, "Server error"});
            } else {
                // Success!
                handler(Response(state->response_impl), Error{});
            }
        }
    });
}

} // namespace http2client
