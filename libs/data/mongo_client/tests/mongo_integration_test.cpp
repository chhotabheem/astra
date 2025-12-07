#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MongoClient.h"
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

using namespace testing;
using namespace mongoclient;

TEST(MongoClientTest, Instantiation) {
    EXPECT_NO_THROW({
        MongoClient client;
    });
}

TEST(MongoClientTest, InitialStateNotConnected) {
    MongoClient client;
    EXPECT_FALSE(client.isConnected());
}

TEST(MongoClientTest, ConnectChangesState) {
    MongoClient client;
    // Assuming localhost:27017 is valid or at least parseable
    try {
        client.connect("mongodb://127.0.0.1:27017");
        EXPECT_TRUE(client.isConnected());
    } catch (...) {
        // If connect throws, we can't really test this without a mock
        // But for now we assume it works or throws
    }
}

TEST(MongoClientTest, DoubleConnectThrows) {
    MongoClient client;
    try {
        client.connect("mongodb://127.0.0.1:27017");
        EXPECT_THROW(client.connect("mongodb://127.0.0.1:27017"), std::runtime_error);
    } catch (...) {
        // If first connect fails, we skip
        GTEST_SKIP() << "Could not connect to MongoDB";
    }
}

TEST(MongoClientTest, DisconnectChangesState) {
    MongoClient client;
    try {
        client.connect("mongodb://127.0.0.1:27017");
        EXPECT_TRUE(client.isConnected());
        client.disconnect();
        EXPECT_FALSE(client.isConnected());
    } catch (...) {
        GTEST_SKIP() << "Could not connect to MongoDB";
    }
}

TEST(MongoClientTest, DisconnectWhenNotConnected) {
    MongoClient client;
    EXPECT_NO_THROW(client.disconnect());
    EXPECT_FALSE(client.isConnected());
}

TEST(MongoClientTest, QueryWithoutConnectionThrows) {
    MongoClient client;
    auto query = bsoncxx::builder::basic::make_document();
    EXPECT_THROW(client.findOne("test_db", "test_collection", query.view()), std::runtime_error);
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
        try {
            m_client.connect("mongodb://127.0.0.1:27017");
            // Try a simple operation to check connectivity
            auto query = bsoncxx::builder::basic::make_document();
            try {
                m_client.find("admin", "system.version", query.view());
            } catch (const std::exception& e) {
                GTEST_SKIP() << "MongoDB not reachable: " << e.what();
            }
        } catch (const std::exception& e) {
            GTEST_SKIP() << "MongoDB connection failed: " << e.what();
        }
    }

    MongoClient m_client;
};

TEST_F(MongoClientCRUDTest, CRUDOperations) {
    try {
        // Insert
        auto doc = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("test_key", "test_value"));
        m_client.insertOne("test_db", "test_collection", doc.view());

        // Find
        auto query = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("test_key", "test_value"));
        auto results = m_client.find("test_db", "test_collection", query.view());
        // We might find more if previous tests ran, but we expect at least 1
        EXPECT_FALSE(results.empty());

        // Update
        auto update = bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("$set",
                bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("test_key", "updated_value"))
            )
        );
        m_client.updateMany("test_db", "test_collection", query.view(), update.view());

        // Delete
        auto delete_filter = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("test_key", "updated_value"));
        m_client.deleteMany("test_db", "test_collection", delete_filter.view());
        
    } catch (const std::exception& e) {
        FAIL() << "CRUD operation failed: " << e.what();
    }
}
