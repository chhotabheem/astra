#include "SwRedisClient.h"

#include <Log.h>

namespace astra::redis {

SwRedisClient::SwRedisClient(const std::string &uri) {
  try {
    m_redis = std::make_unique<sw::redis::Redis>(uri);
  } catch (const std::exception &e) {
    obs::error("Failed to initialize Redis client", {{"error", e.what()}});
    throw;
  }
}

void SwRedisClient::set(const std::string &key, const std::string &value) {
  try {
    m_redis->set(key, value);
  } catch (const std::exception &e) {
    obs::error("Redis set error", {{"error", e.what()}});
    throw;
  }
}

std::optional<std::string> SwRedisClient::get(const std::string &key) const {
  try {
    auto val = m_redis->get(key);
    if (val) {
      return *val;
    }
    return std::nullopt;
  } catch (const std::exception &e) {
    obs::error("Redis get error", {{"error", e.what()}});
    throw;
  }
}

bool SwRedisClient::del(const std::string &key) {
  try {
    return m_redis->del(key) > 0;
  } catch (const std::exception &e) {
    obs::error("Redis del error", {{"error", e.what()}});
    throw;
  }
}

int64_t SwRedisClient::incr(const std::string &key) {
  try {
    return m_redis->incr(key);
  } catch (const std::exception &e) {
    obs::error("Redis incr error", {{"error", e.what()}});
    throw;
  }
}

bool SwRedisClient::ping() const {
  try {
    return m_redis->ping() == "PONG";
  } catch (const std::exception &e) {
    obs::error("Redis ping error", {{"error", e.what()}});
    return false;
  }
}

} // namespace astra::redis
