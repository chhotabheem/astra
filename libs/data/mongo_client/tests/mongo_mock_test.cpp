#include "MockMongoClient.h"
#include <gtest/gtest.h>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>

using namespace mongoclient;
using ::testing::Return;
using ::testing::_;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

class MongoMockTest : public ::testing::Test {
protected:
    MockMongoClient mock_mongo;
};

TEST_F(MongoMockTest, IsConnectedReturnsTrue) {
    EXPECT_CALL(mock_mongo, isConnected())
        .WillOnce(Return(true));

    EXPECT_TRUE(mock_mongo.isConnected());
}

TEST_F(MongoMockTest, FindOneReturnsDocument) {
    auto doc = make_document(kvp("name", "test"));
    
    EXPECT_CALL(mock_mongo, findOne("test_db", "test_col", _))
        .WillOnce(Return(std::optional<bsoncxx::document::value>(doc)));

    auto result = mock_mongo.findOne("test_db", "test_col", make_document());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(bsoncxx::to_json(*result), bsoncxx::to_json(doc));
}

TEST_F(MongoMockTest, InsertOneCallsUnderlying) {
    EXPECT_CALL(mock_mongo, insertOne("test_db", "test_col", _))
        .Times(1);

    mock_mongo.insertOne("test_db", "test_col", make_document(kvp("key", "value")));
}

TEST_F(MongoMockTest, FindReturnsVector) {
    std::vector<bsoncxx::document::value> docs;
    docs.push_back(make_document(kvp("id", 1)));
    docs.push_back(make_document(kvp("id", 2)));

    EXPECT_CALL(mock_mongo, find("test_db", "test_col", _))
        .WillOnce(Return(docs));

    auto result = mock_mongo.find("test_db", "test_col", make_document());
    EXPECT_EQ(result.size(), 2);
}
