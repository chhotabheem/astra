#include "MockRedisClient.h"

#include <gtest/gtest.h>

using namespace astra::redis;
using ::testing::_;
using ::testing::Return;

class RedisMockTest : public ::testing::Test {
protected:
  MockRedisClient mock_redis;
};

TEST_F(RedisMockTest, GetReturnsValue) {
  EXPECT_CALL(mock_redis, get("test_key")).WillOnce(Return("test_value"));

  auto result = mock_redis.get("test_key");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "test_value");
}

TEST_F(RedisMockTest, GetReturnsNullopt) {
  EXPECT_CALL(mock_redis, get("missing_key")).WillOnce(Return(std::nullopt));

  auto result = mock_redis.get("missing_key");
  ASSERT_FALSE(result.has_value());
}

TEST_F(RedisMockTest, SetCallsUnderlying) {
  EXPECT_CALL(mock_redis, set("key", "value")).Times(1);

  mock_redis.set("key", "value");
}

TEST_F(RedisMockTest, PingReturnsTrue) {
  EXPECT_CALL(mock_redis, ping()).WillOnce(Return(true));

  EXPECT_TRUE(mock_redis.ping());
}
