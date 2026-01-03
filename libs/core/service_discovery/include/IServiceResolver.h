#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace astra::service_discovery {

/**
 * @brief Interface for service resolution
 *
 * Abstracts how service endpoints are discovered.
 * Implementations may use static config, Zookeeper, Consul, DNS, etc.
 */
class IServiceResolver {
public:
  virtual ~IServiceResolver() = default;

  /**
   * @brief Resolve a service name to host:port
   * @param service_name The logical service name
   * @return Pair of (host, port)
   * @throws std::runtime_error if service not found
   */
  virtual std::pair<std::string, uint16_t>
  resolve(const std::string &service_name) = 0;

  /**
   * @brief Check if a service is registered
   * @param service_name The logical service name
   * @return true if service exists
   */
  virtual bool has_service(const std::string &service_name) const = 0;
};

} // namespace astra::service_discovery
