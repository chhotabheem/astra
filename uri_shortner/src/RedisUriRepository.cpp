#include "RedisUriRepository.h"
#include <stdexcept>

namespace uri_shortener {

const std::string RedisUriRepository::ID_KEY = "global:url_id";
const std::string RedisUriRepository::URL_PREFIX = "url:";

RedisUriRepository::RedisUriRepository(std::shared_ptr<redisclient::RedisClient> redis)
    : redis_(std::move(redis)) {
    if (!redis_) {
        throw std::invalid_argument("RedisClient cannot be null");
    }
}

uint64_t RedisUriRepository::generate_id() {
    return static_cast<uint64_t>(redis_->incr(ID_KEY));
}

void RedisUriRepository::save(const std::string& short_code, const std::string& long_url) {
    redis_->set(URL_PREFIX + short_code, long_url);
}

std::optional<std::string> RedisUriRepository::find(const std::string& short_code) {
    return redis_->get(URL_PREFIX + short_code);
}

} // namespace uri_shortener
