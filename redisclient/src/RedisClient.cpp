#include "RedisClient.h"
#include <iostream>

namespace redisclient {

RedisClient::RedisClient(const std::string& uri) {
    try {
        redis_ = std::make_unique<sw::redis::Redis>(uri);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Redis client: " << e.what() << std::endl;
        throw;
    }
}

RedisClient::~RedisClient() = default;

void RedisClient::set(std::string_view key, std::string_view value) {
    try {
        redis_->set(key, value);
    } catch (const std::exception& e) {
        std::cerr << "Redis set error: " << e.what() << std::endl;
        throw;
    }
}

std::optional<std::string> RedisClient::get(std::string_view key) {
    try {
        auto val = redis_->get(key);
        if (val) {
            return *val;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Redis get error: " << e.what() << std::endl;
        throw;
    }
}

bool RedisClient::del(std::string_view key) {
    try {
        return redis_->del(key) > 0;
    } catch (const std::exception& e) {
        std::cerr << "Redis del error: " << e.what() << std::endl;
        throw;
    }
}

long long RedisClient::incr(std::string_view key) {
    try {
        return redis_->incr(key);
    } catch (const std::exception& e) {
        std::cerr << "Redis incr error: " << e.what() << std::endl;
        throw;
    }
}

bool RedisClient::ping() {
    try {
        return redis_->ping() == "PONG";
    } catch (const std::exception& e) {
        std::cerr << "Redis ping error: " << e.what() << std::endl;
        return false;
    }
}

} // namespace redisclient
