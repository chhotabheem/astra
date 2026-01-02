#pragma once

#include "IRequest.h"
#include "IResponse.h"
#include "Router.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace astra::http1 {

class Server {
public:
  using Handler = std::function<void(astra::router::IRequest &,
                                     astra::router::IResponse &)>;

  Server(const std::string &address, unsigned short port, int threads = 1);
  ~Server();

  void handle(Handler handler);
  void run();
  void stop();

  astra::router::Router &router() {
    return m_router;
  }

private:
  void do_accept();
  void do_session(boost::asio::ip::tcp::socket socket);

  std::string m_address;
  unsigned short m_port;
  int m_threads;
  boost::asio::io_context m_ioc;
  astra::router::Router m_router;
  boost::asio::ip::tcp::acceptor m_acceptor;
  std::vector<std::thread> m_thread_pool;
  Handler m_handler;
  mutable std::mutex m_handler_mutex;
};

} // namespace astra::http1
