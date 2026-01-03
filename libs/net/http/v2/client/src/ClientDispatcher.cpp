#include "ClientDispatcher.h"

#include <Log.h>

namespace astra::http2 {

ClientDispatcher::ClientDispatcher(const ClientConfig &config)
    : m_config(config) {
  m_work = std::make_unique<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      boost::asio::make_work_guard(m_io));
  m_io_thread = std::thread([this]() {
    try {
      m_io.run();
    } catch (const std::exception &e) {
      obs::error(std::string("ClientDispatcher IO error: ") + e.what());
    }
  });
}

ClientDispatcher::~ClientDispatcher() {
  m_work.reset();
  m_io.stop();
  if (m_io_thread.joinable()) {
    m_io_thread.join();
  }
}

void ClientDispatcher::submit(const std::string &host, uint16_t port,
                              const std::string &method,
                              const std::string &path, const std::string &body,
                              const std::map<std::string, std::string> &headers,
                              ResponseHandler handler) {
  boost::asio::post(m_io, [=]() {
    auto *client = get_or_create(host, port);
    client->submit(method, path, body, headers, handler);
  });
}

NgHttp2Client *ClientDispatcher::get_or_create(const std::string &host,
                                               uint16_t port) {
  std::string key = host + ":" + std::to_string(port);
  auto it = m_clients.find(key);
  if (it == m_clients.end()) {
    auto client = std::make_unique<NgHttp2Client>(
        host, port, m_config,
        [this, key]() {
          remove_client(key);
        },
        [this, key](Http2ClientError) {
          remove_client(key);
        });
    it = m_clients.emplace(key, std::move(client)).first;
  }
  return it->second.get();
}

void ClientDispatcher::remove_client(const std::string &key) {
  boost::asio::post(m_io, [this, key]() {
    m_clients.erase(key);
  });
}

} // namespace astra::http2
