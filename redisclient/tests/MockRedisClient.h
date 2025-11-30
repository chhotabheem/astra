#pragma once

#include "IRedisClient.h"
#include <gmock/gmock.h>

namespace redisclient {

class MockRedisClient : public IRedisClient {
public:
    MOCK_METHOD(void, set, (std::string_view key, std::string_view value), (override));
    MOCK_METHOD(std::optional<std::string>, get, (std::string_view key), (override));
    MOCK_METHOD(bool, del, (std::string_view key), (override));
    MOCK_METHOD(long long, incr, (std::string_view key), (override));
    MOCK_METHOD(bool, ping, (), (override));
};

} // namespace redisclient
