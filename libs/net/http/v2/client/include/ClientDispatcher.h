#pragma once

#include "NgHttp2Client.h"

#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <unordered_map>

namespace astra::http2 {

class ClientDispatcher {
public:
  explicit ClientDispatcher(const ClientConfig &config);
  ~ClientDispatcher();

  ClientDispatcher(const ClientDispatcher &) = delete;
  ClientDispatcher &operator=(const ClientDispatcher &) = delete;

  void submit(const std::string &host, uint16_t port, const std::string &method,
              const std::string &path, const std::string &body,
              const std::map<std::string, std::string> &headers,
              ResponseHandler handler);

private:
  NgHttp2Client *get_or_create(const std::string &host, uint16_t port);
  void remove_client(const std::string &key);

  boost::asio::io_context m_io;
  std::unique_ptr<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>
      m_work;
  std::thread m_io_thread;

  std::unordered_map<std::string, std::unique_ptr<NgHttp2Client>> m_clients;
  ClientConfig m_config;
};

} // namespace astra::http2
