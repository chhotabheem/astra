#pragma once

namespace astra::execution {

class IScopedResource {
public:
  virtual ~IScopedResource() = default;

  IScopedResource() = default;
  IScopedResource(const IScopedResource &) = delete;
  IScopedResource &operator=(const IScopedResource &) = delete;
  IScopedResource(IScopedResource &&) = delete;
  IScopedResource &operator=(IScopedResource &&) = delete;
};

} // namespace astra::execution
