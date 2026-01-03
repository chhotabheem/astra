#pragma once
#include "observability.pb.h"

#include <memory>
#include <string>

namespace astra::observability {

class ProviderImpl;
class Tracer;

class Provider {
public:
  static Provider &instance();

  // Initialize with proto config
  bool init(const ::observability::Config &config);
  bool shutdown();

  // Get tracer for creating spans
  std::shared_ptr<Tracer> get_tracer(const std::string &name);

  // Public alias for accessing implementation (needed by Metrics.cpp)
  using Impl = ProviderImpl;
  Impl &impl();

private:
  Provider();
  ~Provider();
  Provider(const Provider &) = delete;
  Provider &operator=(const Provider &) = delete;

  std::unique_ptr<ProviderImpl> m_impl;
};

// Convenience functions
bool init(const ::observability::Config &config);
bool shutdown();

} // namespace astra::observability

// Backward compatibility alias
namespace obs = astra::observability;
