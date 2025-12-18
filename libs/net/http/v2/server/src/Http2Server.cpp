#include "Http2Server.h"
#include "Http2Request.h"
#include "Http2Response.h"
#include "backend/nghttp2/NgHttp2Server.h"

namespace astra::http2 {

class Server::Impl {
public:
    Impl(const ServerConfig& config)
        : backend(config) {}
    
    backend::NgHttp2Server backend;
};

Server::Server(const ServerConfig& config)
    : m_impl(std::make_unique<Impl>(config)) {
    
    // Default handler: Dispatch to Router with shared_ptr
    m_impl->backend.handle("*", "/", [this](std::shared_ptr<astra::router::IRequest> req, 
                                             std::shared_ptr<astra::router::IResponse> res) {
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

} // namespace astra::http2
