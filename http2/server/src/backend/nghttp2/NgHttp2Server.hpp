#pragma once

#include <string>
#include <nghttp2/asio_http2_server.h>
#include "Http2Server.hpp"

namespace http2server {
namespace backend {

class NgHttp2Server {
public:
    NgHttp2Server(const std::string& address, const std::string& port, int threads);
    ~NgHttp2Server();

    void handle(const std::string& method, const std::string& path, Server::Handler handler);
    void run();
    void stop();

private:
    std::string address_;
    std::string port_;
    int threads_;
    bool is_running_ = false;
    nghttp2::asio_http2::server::http2 server_;
};

} // namespace backend
} // namespace http2server
