#pragma once

#include <string>
#include <atomic>
#include <future>
#include <nghttp2/asio_http2_server.h>
#include "Http2Server.h"

namespace http2server {
namespace backend {

class NgHttp2Server {
public:
    NgHttp2Server(const std::string& address, const std::string& port, int threads);
    ~NgHttp2Server();

    void handle(const std::string& method, const std::string& path, Server::Handler handler);
    void run();
    void stop();
    
    /// Block until the server is ready to accept connections
    void wait_until_ready();

private:
    std::string m_address;
    std::string m_port;
    int m_threads;
    std::atomic<bool> is_running_{false};
    nghttp2::asio_http2::server::http2 server_;
    
    // Ready signaling (promise set once, future can be waited on)
    std::promise<void> ready_promise_;
    std::shared_future<void> ready_future_;
};

} // namespace backend
} // namespace http2server

