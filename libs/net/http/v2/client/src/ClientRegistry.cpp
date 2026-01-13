#include "ClientRegistry.h"

namespace astra::http2 {

ClientRegistry::ClientRegistry(const ::http2::ClientConfig &config)
    : m_config(config) {
}

ClientRegistry::~ClientRegistry() = default;

std::shared_ptr<NgHttp2Client>
ClientRegistry::get_or_create(const std::string &host, uint16_t port) {
  std::string key = host + ":" + std::to_string(port);

  {
    std::shared_lock lock(m_mutex);
    auto it = m_clients.find(key);
    if (it != m_clients.end() && !it->second->is_dead()) {
      return it->second;
    }
  }

  std::unique_lock lock(m_mutex);
  auto it = m_clients.find(key);
  if (it != m_clients.end() && it->second->is_dead()) {
    m_clients.erase(it);
    it = m_clients.end();
  }
  if (it == m_clients.end()) {
    auto client =
        std::make_shared<NgHttp2Client>(host, port, m_config, nullptr, nullptr);
    it = m_clients.emplace(key, std::move(client)).first;
  }
  return it->second;
}

} // namespace astra::http2
