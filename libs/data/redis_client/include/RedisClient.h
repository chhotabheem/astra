#pragma once

#include "IRedisClient.h"

#include <string>
#include <optional>
#include <memory>
#include <sw/redis++/redis++.h>

namespace redisclient {

class RedisClient : public IRedisClient {
public:
    explicit RedisClient(const std::string& uri);
    ~RedisClient() override;

    // Delete copy constructor and assignment operator
    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    // Basic operations
    void set(std::string_view key, std::string_view value) override;
    [[nodiscard]] std::optional<std::string> get(std::string_view key) override;
    bool del(std::string_view key) override;
    long long incr(std::string_view key) override;
    
    // Check connection
    bool ping() override;

private:
    std::unique_ptr<sw::redis::Redis> redis_;
};

} // namespace redisclient
