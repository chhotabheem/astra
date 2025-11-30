#pragma once

#include <string>
#include <string_view>
#include <optional>

namespace redisclient {

class IRedisClient {
public:
    virtual ~IRedisClient() = default;

    // Basic operations
    virtual void set(std::string_view key, std::string_view value) = 0;
    [[nodiscard]] virtual std::optional<std::string> get(std::string_view key) = 0;
    virtual bool del(std::string_view key) = 0;
    virtual long long incr(std::string_view key) = 0;
    
    // Check connection
    virtual bool ping() = 0;
};

} // namespace redisclient
