#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "RedisClient.h"

using namespace testing;

class RedisClientTest : public Test {
protected:
    void SetUp() override {
        try {
            client_ = std::make_unique<redisclient::RedisClient>("tcp://127.0.0.1:6379");
            if (!client_->ping()) {
                GTEST_SKIP() << "Redis server not available at tcp://127.0.0.1:6379";
            }
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Redis connection failed: " << e.what();
        }
    }

    std::unique_ptr<redisclient::RedisClient> client_;
};

TEST_F(RedisClientTest, SetAndGet) {
    if (!client_) return; // Skipped in SetUp

    client_->set("test_key", "test_value");
    
    auto val = client_->get("test_key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "test_value");
}

TEST_F(RedisClientTest, Delete) {
    if (!client_) return;

    client_->set("test_key_del", "value");
    bool deleted = client_->del("test_key_del");
    EXPECT_TRUE(deleted);
    
    auto val = client_->get("test_key_del");
    EXPECT_FALSE(val.has_value());
}

TEST_F(RedisClientTest, GetMissing) {
    if (!client_) return;

    auto val = client_->get("non_existent_key");
    EXPECT_FALSE(val.has_value());
}
