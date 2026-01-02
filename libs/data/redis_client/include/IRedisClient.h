#pragma once

#include <optional>
#include <string>

namespace astra::redis {

class IRedisClient {
public:
  virtual ~IRedisClient() = default;

  // Basic operations
  virtual void set(const std::string &key, const std::string &value) = 0;
  [[nodiscard]] virtual std::optional<std::string>
  get(const std::string &key) = 0;
  virtual bool del(const std::string &key) = 0;
  virtual long long incr(const std::string &key) = 0;

  // Check connection
  virtual bool ping() = 0;
};

} // namespace astra::redis
