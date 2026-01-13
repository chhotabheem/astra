#pragma once

#include "NgHttp2Client.h"

#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace astra::http2 {

class ClientRegistry {
public:
  explicit ClientRegistry(const ::http2::ClientConfig &config);
  ~ClientRegistry();

  ClientRegistry(const ClientRegistry &) = delete;
  ClientRegistry &operator=(const ClientRegistry &) = delete;

  std::shared_ptr<NgHttp2Client> get_or_create(const std::string &host,
                                               uint16_t port);

private:
  std::unordered_map<std::string, std::shared_ptr<NgHttp2Client>> m_clients;
  mutable std::shared_mutex m_mutex;
  ::http2::ClientConfig m_config;
};

} // namespace astra::http2
