#pragma once

#include "IServiceResolver.h"

#include <stdexcept>
#include <unordered_map>

namespace astra::service_discovery {

/**
 * @brief Static/hardcoded service resolver
 *
 * Simple implementation for development and testing.
 * Services are registered programmatically.
 */
class StaticServiceResolver : public IServiceResolver {
public:
  StaticServiceResolver() = default;
  ~StaticServiceResolver() override = default;

  /**
   * @brief Register a service with its endpoint
   * @param service_name Logical service name
   * @param host Host address (IP or hostname)
   * @param port Port number
   * @throws std::invalid_argument if service_name or host is empty
   */
  void register_service(const std::string &service_name,
                        const std::string &host, uint16_t port);

  /**
   * @brief Unregister a service
   * @param service_name Logical service name
   */
  void unregister_service(const std::string &service_name);

  std::pair<std::string, uint16_t>
  resolve(const std::string &service_name) override;
  bool has_service(const std::string &service_name) const override;

private:
  struct Endpoint {
    std::string host;
    uint16_t port;
  };

  std::unordered_map<std::string, Endpoint> m_services;
};

} // namespace astra::service_discovery
