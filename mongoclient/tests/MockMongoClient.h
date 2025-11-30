#pragma once

#include "IMongoClient.h"
#include <gmock/gmock.h>

namespace mongoclient {

class MockMongoClient : public IMongoClient {
public:
    MOCK_METHOD(void, connect, (const std::string& uri), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    
    MOCK_METHOD(std::optional<bsoncxx::document::value>, findOne, (
        const std::string& database, 
        const std::string& collection,
        const bsoncxx::document::view& query), (override));

    MOCK_METHOD(void, insertOne, (
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& document), (override));

    MOCK_METHOD(void, insertMany, (
        const std::string& database,
        const std::string& collection,
        const std::vector<bsoncxx::document::value>& documents), (override));

    MOCK_METHOD(void, updateMany, (
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter,
        const bsoncxx::document::view& update), (override));

    MOCK_METHOD(void, deleteMany, (
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter), (override));

    MOCK_METHOD(std::vector<bsoncxx::document::value>, find, (
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& query), (override));
};

} // namespace mongoclient
