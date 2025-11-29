#include "NgHttp2Server.hpp"
#include "Http2Request.hpp"
#include "Http2Response.hpp"
#include "../../RequestImpl.hpp"
#include "../../ResponseImpl.hpp"
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

        struct Context {
            Request::Impl req_impl;
            const nghttp2::asio_http2::server::response* res;
            Server::Handler handler;
        };
        
        auto ctx = std::make_shared<Context>();
        ctx->req_impl.method = req.method();
        ctx->req_impl.path = req.uri().path;
        for (const auto& h : req.header()) {
            ctx->req_impl.headers[h.first] = h.second.value;
        }
        ctx->res = &res;
        ctx->handler = handler;

        const_cast<nghttp2::asio_http2::server::request&>(req).on_data([ctx](const uint8_t *data, std::size_t len) {
            if (len > 0) {
                ctx->req_impl.body.append(reinterpret_cast<const char*>(data), len);
            } else {
                Request request;
                request.m_impl = std::make_unique<Request::Impl>(ctx->req_impl);
                
                Response response;
                response.m_impl = std::make_unique<Response::Impl>(*ctx->res);
                
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
