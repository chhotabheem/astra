#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <sw/redis++/redis++.h>

namespace astra::redis {

class SwRedisClient {
public:
  explicit SwRedisClient(const std::string &uri);
  ~SwRedisClient() = default;

  void set(const std::string &key, const std::string &value);
  [[nodiscard]] std::optional<std::string> get(const std::string &key) const;
  [[nodiscard]] bool del(const std::string &key);
  [[nodiscard]] int64_t incr(const std::string &key);
  [[nodiscard]] bool ping() const;

private:
  std::unique_ptr<sw::redis::Redis> m_redis;
};

} // namespace astra::redis
