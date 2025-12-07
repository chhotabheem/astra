#pragma once

#include "IRequest.h"
#include "IResponse.h"
#include "Router.h"
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
    using Handler = std::function<void(router::IRequest&, router::IResponse&)>;

    Server(const std::string& address, unsigned short port, int threads = 1);
    ~Server();

    void handle(Handler handler);
    void run();
    void stop();

    router::Router& router() { return m_router; }

private:
    void do_accept();
    void do_session(boost::asio::ip::tcp::socket socket);

    std::string m_address;
    unsigned short m_port;
    int m_threads;
    boost::asio::io_context m_ioc;
    router::Router m_router;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::vector<std::thread> m_thread_pool;
    Handler m_handler;
    mutable std::mutex m_handler_mutex;
};

} // namespace http1
