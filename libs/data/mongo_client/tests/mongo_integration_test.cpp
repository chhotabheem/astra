#include "MongoClient.h"

#include <JsonWriter.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace astra::mongo;

TEST(MongoClientTest, Instantiation) {
  EXPECT_NO_THROW({ MongoClient client; });
}

TEST(MongoClientTest, InitialStateNotConnected) {
  MongoClient client;
  EXPECT_FALSE(client.isConnected());
}

TEST(MongoClientTest, ConnectChangesState) {
  MongoClient client;
  auto result = client.connect("mongodb://127.0.0.1:27017");
  if (result.is_ok()) {
    EXPECT_TRUE(client.isConnected());
  } else {
    // Connection failed - skip
    GTEST_SKIP() << "Could not connect to MongoDB: " << result.error().message;
  }
}

TEST(MongoClientTest, DoubleConnectReturnsError) {
  MongoClient client;
  auto result1 = client.connect("mongodb://127.0.0.1:27017");
  if (result1.is_err()) {
    GTEST_SKIP() << "Could not connect to MongoDB";
  }

  auto result2 = client.connect("mongodb://127.0.0.1:27017");
  EXPECT_TRUE(result2.is_err());
  EXPECT_EQ(result2.error().code, MongoError::Code::AlreadyConnected);
}

TEST(MongoClientTest, DisconnectChangesState) {
  MongoClient client;
  auto result = client.connect("mongodb://127.0.0.1:27017");
  if (result.is_err()) {
    GTEST_SKIP() << "Could not connect to MongoDB";
  }

  EXPECT_TRUE(client.isConnected());
  client.disconnect();
  EXPECT_FALSE(client.isConnected());
}

TEST(MongoClientTest, DisconnectWhenNotConnected) {
  MongoClient client;
  EXPECT_NO_THROW(client.disconnect());
  EXPECT_FALSE(client.isConnected());
}

TEST(MongoClientTest, QueryWithoutConnectionReturnsError) {
  MongoClient client;
  auto result = client.findOne("test_db", "test_collection", "{}");
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.error().code, MongoError::Code::NotConnected);
}

TEST(MongoClientTest, MultipleClientInstances) {
  EXPECT_NO_THROW({
    MongoClient client1;
    MongoClient client2;
    MongoClient client3;
    EXPECT_FALSE(client1.isConnected());
    EXPECT_FALSE(client2.isConnected());
    EXPECT_FALSE(client3.isConnected());
  });
}

class MongoClientCRUDTest : public Test {
protected:
  void SetUp() override {
    auto connectResult = m_client.connect("mongodb://127.0.0.1:27017");
    if (connectResult.is_err()) {
      GTEST_SKIP() << "MongoDB connection failed: "
                   << connectResult.error().message;
    }

    // Try a simple operation to check connectivity
    auto pingResult = m_client.find("admin", "system.version", "{}");
    if (pingResult.is_err()) {
      GTEST_SKIP() << "MongoDB not reachable: " << pingResult.error().message;
    }
  }

  MongoClient m_client;
};

TEST_F(MongoClientCRUDTest, CRUDOperations) {
  // Insert using JsonWriter
  astra::json::JsonWriter writer;
  writer.add("test_key", "test_value");
  std::string docJson = writer.get_string();

  auto insertResult = m_client.insertOne("test_db", "test_collection", docJson);
  ASSERT_TRUE(insertResult.is_ok())
      << "Insert failed: " << insertResult.error().message;

  // Find
  astra::json::JsonWriter queryWriter;
  queryWriter.add("test_key", "test_value");
  std::string queryJson = queryWriter.get_string();

  auto findResult = m_client.find("test_db", "test_collection", queryJson);
  ASSERT_TRUE(findResult.is_ok())
      << "Find failed: " << findResult.error().message;
  EXPECT_FALSE(findResult.value().empty());

  // Update
  std::string updateJson = R"({"$set": {"test_key": "updated_value"}})";
  auto updateResult =
      m_client.updateMany("test_db", "test_collection", queryJson, updateJson);
  ASSERT_TRUE(updateResult.is_ok())
      << "Update failed: " << updateResult.error().message;

  // Delete
  astra::json::JsonWriter deleteWriter;
  deleteWriter.add("test_key", "updated_value");
  std::string deleteFilterJson = deleteWriter.get_string();

  auto deleteResult =
      m_client.deleteMany("test_db", "test_collection", deleteFilterJson);
  ASSERT_TRUE(deleteResult.is_ok())
      << "Delete failed: " << deleteResult.error().message;
}
