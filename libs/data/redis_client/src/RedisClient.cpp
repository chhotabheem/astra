#include "RedisClient.h"

#include "SwRedisClient.h"

namespace astra::redis {

class RedisClient::Impl {
public:
  explicit Impl(const std::string &uri) : backend(uri) {
  }

  SwRedisClient backend;
};

RedisClient::RedisClient(const std::string &uri)
    : m_impl(std::make_unique<Impl>(uri)) {
}

RedisClient::~RedisClient() = default;

void RedisClient::set(const std::string &key, const std::string &value) {
  m_impl->backend.set(key, value);
}

std::optional<std::string> RedisClient::get(const std::string &key) const {
  return m_impl->backend.get(key);
}

bool RedisClient::del(const std::string &key) {
  return m_impl->backend.del(key);
}

int64_t RedisClient::incr(const std::string &key) {
  return m_impl->backend.incr(key);
}

bool RedisClient::ping() const {
  return m_impl->backend.ping();
}

} // namespace astra::redis
