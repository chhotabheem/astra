#include "Http2Server.h"
#include "Http2Request.h"
#include "Http2Response.h"
#include "backend/nghttp2/NgHttp2Server.h"

namespace http2server {

class Server::Impl {
public:
    Impl(const http2server::Config& config)
        : backend(config) {}
    
    backend::NgHttp2Server backend;
};

Server::Server(const http2server::Config& config)
    : m_impl(std::make_unique<Impl>(config)) {
    
    // Default handler: Dispatch to Router
    m_impl->backend.handle("*", "/", [this](Request& req, Response& res) {
        m_router.dispatch(req, res);
    });
}

Server::~Server() = default;

void Server::handle(const std::string& method, const std::string& path, Handler handler) {
    m_impl->backend.handle(method, path, handler);
}

void Server::run() {
    m_impl->backend.run();
}

void Server::stop() {
    m_impl->backend.stop();
}

void Server::wait_until_ready() {
    m_impl->backend.wait_until_ready();
}

} // namespace http2server
