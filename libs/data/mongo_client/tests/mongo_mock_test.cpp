#include "MockMongoClient.h"

#include <gtest/gtest.h>

using namespace astra::mongo;
using ::testing::_;
using ::testing::Return;

class MongoMockTest : public ::testing::Test {
protected:
  MockMongoClient mock_mongo;
};

TEST_F(MongoMockTest, IsConnectedReturnsTrue) {
  EXPECT_CALL(mock_mongo, isConnected()).WillOnce(Return(true));

  EXPECT_TRUE(mock_mongo.isConnected());
}

TEST_F(MongoMockTest, FindOneReturnsDocument) {
  std::string docJson = R"({"name": "test"})";
  auto successResult =
      Result<std::optional<std::string>, MongoError>::Ok(docJson);

  EXPECT_CALL(mock_mongo, findOne("test_db", "test_col", _))
      .WillOnce(Return(successResult));

  auto result = mock_mongo.findOne("test_db", "test_col", "{}");
  ASSERT_TRUE(result.is_ok());
  ASSERT_TRUE(result.value().has_value());
  EXPECT_EQ(result.value().value(), docJson);
}

TEST_F(MongoMockTest, InsertOneCallsUnderlying) {
  EXPECT_CALL(mock_mongo, insertOne("test_db", "test_col", _))
      .WillOnce(Return(Result<void, MongoError>::Ok()));

  auto result =
      mock_mongo.insertOne("test_db", "test_col", R"({"key": "value"})");
  EXPECT_TRUE(result.is_ok());
}

TEST_F(MongoMockTest, FindReturnsVector) {
  std::vector<std::string> docs = {R"({"id": 1})", R"({"id": 2})"};
  auto successResult = Result<std::vector<std::string>, MongoError>::Ok(docs);

  EXPECT_CALL(mock_mongo, find("test_db", "test_col", _))
      .WillOnce(Return(successResult));

  auto result = mock_mongo.find("test_db", "test_col", "{}");
  ASSERT_TRUE(result.is_ok());
  EXPECT_EQ(result.value().size(), 2);
}

TEST_F(MongoMockTest, FindOneReturnsErrorWhenNotConnected) {
  auto errorResult = Result<std::optional<std::string>, MongoError>::Err(
      MongoError{MongoError::Code::NotConnected, "Not connected to MongoDB"});

  EXPECT_CALL(mock_mongo, findOne("test_db", "test_col", _))
      .WillOnce(Return(errorResult));

  auto result = mock_mongo.findOne("test_db", "test_col", "{}");
  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.error().code, MongoError::Code::NotConnected);
}
