#pragma once

#include "IRedisClient.h"

#include <string>
#include <optional>
#include <memory>
#include <sw/redis++/redis++.h>

namespace astra::redis {

class RedisClient : public IRedisClient {
public:
    explicit RedisClient(const std::string& uri);
    ~RedisClient() override;

    // Delete copy constructor and assignment operator
    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    // Basic operations
    void set(const std::string& key, const std::string& value) override;
    [[nodiscard]] std::optional<std::string> get(const std::string& key) override;
    bool del(const std::string& key) override;
    long long incr(const std::string& key) override;
    
    // Check connection
    bool ping() override;

private:
    std::unique_ptr<sw::redis::Redis> redis_;
};

} // namespace astra::redis
