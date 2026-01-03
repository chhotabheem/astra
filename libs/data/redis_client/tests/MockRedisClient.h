#pragma once

#include "IRedisClient.h"

#include <gmock/gmock.h>

namespace astra::redis {

class MockRedisClient : public IRedisClient {
public:
  MOCK_METHOD(void, set, (const std::string &key, const std::string &value),
              (override));
  MOCK_METHOD(std::optional<std::string>, get, (const std::string &key),
              (const, override));
  MOCK_METHOD(bool, del, (const std::string &key), (override));
  MOCK_METHOD(int64_t, incr, (const std::string &key), (override));
  MOCK_METHOD(bool, ping, (), (const, override));
};

} // namespace astra::redis
