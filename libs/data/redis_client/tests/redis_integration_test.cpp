#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "RedisClient.h"

using namespace testing;

class RedisClientTest : public Test {
protected:
    void SetUp() override {
        try {
            m_client = std::make_unique<redisclient::RedisClient>("tcp://127.0.0.1:6379");
            if (!m_client->ping()) {
                GTEST_SKIP() << "Redis server not available at tcp://127.0.0.1:6379";
            }
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Redis connection failed: " << e.what();
        }
    }

    std::unique_ptr<redisclient::RedisClient> m_client;
};

TEST_F(RedisClientTest, SetAndGet) {
    if (!m_client) return; // Skipped in SetUp

    m_client->set("test_key", "test_value");
    
    auto val = m_client->get("test_key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "test_value");
}

TEST_F(RedisClientTest, Delete) {
    if (!m_client) return;

    m_client->set("test_key_del", "value");
    bool deleted = m_client->del("test_key_del");
    EXPECT_TRUE(deleted);
    
    auto val = m_client->get("test_key_del");
    EXPECT_FALSE(val.has_value());
}

TEST_F(RedisClientTest, GetMissing) {
    if (!m_client) return;

    auto val = m_client->get("non_existent_key");
    EXPECT_FALSE(val.has_value());
}
