#include "NgHttp2Server.h"
#include "Http2Request.h"
#include "Http2Response.h"
#include "ResponseHandle.h"
#include "../../RequestImpl.h"
#include "../../ResponseImpl.h"
#include <Logger.h>
#include <iostream>

namespace http2server {
namespace backend {

NgHttp2Server::NgHttp2Server(const std::string& address, const std::string& port, int threads)
    : address_(address), port_(port), threads_(threads) {
    server_.num_threads(threads);
    logger::Logger::info("NgHttp2Server initialized with " + std::to_string(threads) + " threads");
}

NgHttp2Server::~NgHttp2Server() {
    if (is_running_) {
        stop();
    }
}

void NgHttp2Server::handle(const std::string& method, const std::string& path, Server::Handler handler) {
    server_.handle(path, [handler, method](const nghttp2::asio_http2::server::request& req, const nghttp2::asio_http2::server::response& res) {
        if (!method.empty() && method != "*" && req.method() != method) {
            return; 
        }

        // Create context to hold request data and response handle
        struct Context {
            Request::Impl req_impl;
            std::shared_ptr<ResponseHandle> response_handle;
            Server::Handler handler;
        };
        
        auto ctx = std::make_shared<Context>();
        ctx->req_impl.method = req.method();
        ctx->req_impl.path = req.uri().path;
        for (const auto& h : req.header()) {
            ctx->req_impl.headers[h.first] = h.second.value;
        }
        ctx->handler = handler;
        
        // Create ResponseHandle with send function that captures response by reference
        // This is safe because:
        // 1. Send function posts to io_context where response lives
        // 2. Posted lambda is serialized with on_close by event loop
        // 3. Atomic flag prevents sending to closed streams
        auto& io_ctx = res.io_service();
        ctx->response_handle = std::make_shared<ResponseHandle>(
            [&res](std::string data) {
                // This lambda executes on io_context thread
                // Safe to access res here
                res.write_head(200);  // Default 200 OK
                res.end(std::move(data));
            },
            io_ctx
        );
        
        // Register on_close callback to mark stream as closed
        const_cast<nghttp2::asio_http2::server::response&>(res).on_close(
            [response_handle = ctx->response_handle](uint32_t error_code) {
                response_handle->mark_closed();
                if (error_code != 0) {
                    logger::Logger::debug("Stream closed with error code: " + std::to_string(error_code));
                }
            }
        );

        const_cast<nghttp2::asio_http2::server::request&>(req).on_data([ctx](const uint8_t *data, std::size_t len) {
            if (len > 0) {
                ctx->req_impl.body.append(reinterpret_cast<const char*>(data), len);
            } else {
                // Request complete, create Request and Response wrappers
                Request request;
                request.m_impl = std::make_unique<Request::Impl>(ctx->req_impl);
                
                Response response;
                response.m_impl = std::make_unique<Response::Impl>();
                response.m_impl->response_handle = ctx->response_handle;
                
                // Call user handler (can now safely post to worker pool)
                ctx->handler(request, response);
            }
        });
    });
}

void NgHttp2Server::run() {
    is_running_ = true;
    boost::system::error_code ec;
    logger::Logger::info("Server starting on " + address_ + ":" + port_);
    server_.listen_and_serve(ec, address_, port_);
    if (ec) {
        logger::Logger::error("Server stopped with error: " + ec.message());
    } else {
        logger::Logger::info("Server stopped cleanly");
    }
}

void NgHttp2Server::stop() {
    if (is_running_) {
        server_.stop();
        is_running_ = false;
        logger::Logger::info("Server stopped");
    }
}

} // namespace backend
} // namespace http2server
