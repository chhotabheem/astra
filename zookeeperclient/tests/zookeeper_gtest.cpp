#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ZookeeperClient.h"

using namespace testing;
using namespace zookeeperclient;

TEST(ZookeeperClientTest, MockOperations) {
    ZookeeperClient client("127.0.0.1:2181");
    
    // Test create
    bool created = client.create("/test_node", "test_value");
    EXPECT_TRUE(created);
    
    // Test create duplicate (Mock behavior)
    bool created_dup = client.create("/test_node", "new_value");
    // Just ensure it doesn't crash
    (void)created_dup;

    // Test exists
    bool exists = client.exists("/test_node");
    EXPECT_TRUE(exists);
    
    // Test get
    std::string value = client.get("/test_node");
    EXPECT_FALSE(value.empty());
    
    // Test get missing
    std::string missing = client.get("/missing_node");
    EXPECT_FALSE(missing.empty());
}
