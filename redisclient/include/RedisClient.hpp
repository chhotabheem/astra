#pragma once

#include <string>
#include <optional>
#include <memory>
#include <sw/redis++/redis++.h>

namespace redisclient {

class RedisClient {
public:
    explicit RedisClient(const std::string& uri);
    ~RedisClient();

    // Delete copy constructor and assignment operator
    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    // Basic operations
    void set(std::string_view key, std::string_view value);
    [[nodiscard]] std::optional<std::string> get(std::string_view key);
    bool del(std::string_view key);
    long long incr(std::string_view key);
    
    // Check connection
    bool ping();

private:
    std::unique_ptr<sw::redis::Redis> redis_;
};

} // namespace redisclient
