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
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    
    // Check connection
    bool ping();

private:
    std::unique_ptr<sw::redis::Redis> redis_;
};

} // namespace redisclient
