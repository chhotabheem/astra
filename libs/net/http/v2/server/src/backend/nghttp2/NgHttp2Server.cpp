#include "NgHttp2Server.h"
#include "Http2Request.h"
#include "Http2Response.h"
#include "ResponseHandle.h"
#include "RequestData.h"
#include <obs/Log.h>
#include <iostream>

namespace http2server {
namespace backend {

NgHttp2Server::NgHttp2Server(const std::string& address, const std::string& port, int threads)
    : m_address(address), m_port(port), m_threads(threads),
      ready_future_(ready_promise_.get_future().share()) {
    server_.num_threads(m_threads);
    obs::info("NgHttp2Server initialized with " + std::to_string(m_threads) + " threads");
}

NgHttp2Server::~NgHttp2Server() {
    if (is_running_.load(std::memory_order_acquire)) {
        stop();
    }
}

void NgHttp2Server::handle(const std::string& method, const std::string& path, Server::Handler handler) {
    server_.handle(path, [handler, method](const nghttp2::asio_http2::server::request& req, const nghttp2::asio_http2::server::response& res) {
        if (!method.empty() && method != "*" && req.method() != method) {
            return; 
        }

        // Create context to hold request data and response handle
        // Context owns the shared_ptr, keeping data alive until stream closes
        struct Context {
            std::shared_ptr<RequestData> request_data;
            std::shared_ptr<ResponseHandle> response_handle;
            Server::Handler handler;
        };
        
        auto ctx = std::make_shared<Context>();
        ctx->request_data = std::make_shared<RequestData>();
        ctx->request_data->method = req.method();
        ctx->request_data->path = req.uri().path;
        for (const auto& h : req.header()) {
            ctx->request_data->headers[h.first] = h.second.value;
        }
        ctx->handler = handler;
        
        // Create ResponseHandle with send function that captures response by reference
        // This is safe because:
        // 1. Send function posts to io_context where response lives
        // 2. Posted lambda is serialized with on_close by event loop
        // 3. Atomic flag prevents sending to closed streams
        auto& io_ctx = res.io_service();
        ctx->response_handle = std::make_shared<ResponseHandle>(
            [&res](int status, std::map<std::string, std::string> headers, std::string body) {
                // This lambda executes on io_context thread
                // Safe to access res here
                
                // Convert headers to nghttp2 format
                nghttp2::asio_http2::header_map h;
                for (const auto& [k, v] : headers) {
                    h.emplace(k, nghttp2::asio_http2::header_value{v, false});
                }
                
                res.write_head(status, h);
                res.end(std::move(body));
            },
            io_ctx
        );
        
        // Register on_close callback to mark stream as closed
        const_cast<nghttp2::asio_http2::server::response&>(res).on_close(
            [response_handle = ctx->response_handle](uint32_t error_code) {
                response_handle->mark_closed();
                if (error_code != 0) {
                    obs::debug("Stream closed with error code: " + std::to_string(error_code));
                }
            }
        );

        const_cast<nghttp2::asio_http2::server::request&>(req).on_data([ctx](const uint8_t *data, std::size_t len) {
            if (len > 0) {
                ctx->request_data->body.append(reinterpret_cast<const char*>(data), len);
            } else {
                // Request complete, create lightweight Request and Response handles
                Request request(ctx->request_data);
                Response response(ctx->response_handle);
                
                // Call user handler with references
                ctx->handler(request, response);
            }
        });
    });
}

void NgHttp2Server::run() {
    obs::info("Server starting on " + m_address + ":" + m_port);
    
    boost::system::error_code ec;
    
    // Start server in async mode - this creates acceptors and returns immediately
    if (server_.listen_and_serve(ec, m_address, m_port, true /*asynchronous*/)) {
        obs::error("Server failed to start: " + ec.message());
        return;
    }
    
    // At this point, acceptors are created and io_services are running
    // Safe to signal ready and allow stop() to be called
    is_running_.store(true, std::memory_order_release);
    ready_promise_.set_value();
    
    // Block until server stops
    server_.join();
    
    obs::info("Server stopped cleanly");
}

void NgHttp2Server::stop() {
    if (is_running_.load(std::memory_order_acquire)) {
        // Post stop to io_context for thread-safe shutdown
        // This ensures acceptor close happens on the same thread as async operations
        auto& io_services = server_.io_services();
        if (!io_services.empty()) {
            boost::asio::post(*io_services[0], [this]() {
                server_.stop();
            });
        } else {
            server_.stop();
        }
        is_running_.store(false, std::memory_order_release);
        obs::info("Server stopped");
    }
}

void NgHttp2Server::wait_until_ready() {
    ready_future_.wait();
}

} // namespace backend
} // namespace http2server
