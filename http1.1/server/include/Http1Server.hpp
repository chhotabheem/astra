#pragma once

#include "http_abstractions/IRequest.hpp"
#include "http_abstractions/IResponse.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <functional>

namespace http1 {

class Server {
public:
    using Handler = std::function<void(const http_abstractions::IRequest&, http_abstractions::IResponse&)>;

    Server(const std::string& address, unsigned short port, int threads = 1);
    ~Server();

    // Register a handler for all requests (typically the Router dispatch)
    void handle(Handler handler);

    void run();
    void stop();

private:
    void do_accept();
    void do_session(boost::asio::ip::tcp::socket socket);

    std::string address_;
    unsigned short port_;
    int threads_;
    boost::asio::io_context ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::vector<std::thread> thread_pool_;
    Handler handler_;
};

} // namespace http1
