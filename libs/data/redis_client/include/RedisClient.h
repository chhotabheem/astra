#pragma once

#include "IRedisClient.h"

#include <memory>
#include <optional>
#include <string>

namespace astra::redis {

class RedisClient : public IRedisClient {
public:
  explicit RedisClient(const std::string &uri);
  ~RedisClient() override;

  RedisClient(const RedisClient &) = delete;
  RedisClient &operator=(const RedisClient &) = delete;

  void set(const std::string &key, const std::string &value) override;
  [[nodiscard]] std::optional<std::string>
  get(const std::string &key) const override;
  [[nodiscard]] bool del(const std::string &key) override;
  [[nodiscard]] int64_t incr(const std::string &key) override;
  [[nodiscard]] bool ping() const override;

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace astra::redis
