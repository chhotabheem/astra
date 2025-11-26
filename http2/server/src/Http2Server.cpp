#include "Http2Server.hpp"
#include "Http2Request.hpp"
#include "Http2Response.hpp"
#include "backend/nghttp2/NgHttp2Server.hpp"

namespace http2server {

class Server::Impl {
public:
    Impl(const std::string& address, const std::string& port, int threads)
        : backend(address, port, threads) {}
    
    backend::NgHttp2Server backend;
};

Server::Server(const std::string& address, const std::string& port, int threads)
    : m_impl(std::make_unique<Impl>(address, port, threads)) {}

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

} // namespace http2server
