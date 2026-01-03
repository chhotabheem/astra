#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace astra::redis {

class IRedisClient {
public:
  virtual ~IRedisClient() = default;

  virtual void set(const std::string &key, const std::string &value) = 0;
  [[nodiscard]] virtual std::optional<std::string>
  get(const std::string &key) const = 0;
  [[nodiscard]] virtual bool del(const std::string &key) = 0;
  [[nodiscard]] virtual int64_t incr(const std::string &key) = 0;
  [[nodiscard]] virtual bool ping() const = 0;
};

} // namespace astra::redis
