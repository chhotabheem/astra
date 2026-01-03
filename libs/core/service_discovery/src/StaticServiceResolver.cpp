#include "StaticServiceResolver.h"

namespace astra::service_discovery {

void StaticServiceResolver::register_service(const std::string &service_name,
                                             const std::string &host,
                                             uint16_t port) {
  if (service_name.empty()) {
    throw std::invalid_argument("Service name cannot be empty");
  }
  if (host.empty()) {
    throw std::invalid_argument("Host cannot be empty");
  }

  m_services[service_name] = Endpoint{host, port};
}

void StaticServiceResolver::unregister_service(
    const std::string &service_name) {
  m_services.erase(service_name);
}

std::pair<std::string, uint16_t>
StaticServiceResolver::resolve(const std::string &service_name) {
  auto it = m_services.find(service_name);
  if (it == m_services.end()) {
    throw std::runtime_error("Service not found: " + service_name);
  }
  return {it->second.host, it->second.port};
}

bool StaticServiceResolver::has_service(const std::string &service_name) const {
  return m_services.find(service_name) != m_services.end();
}

} // namespace astra::service_discovery
