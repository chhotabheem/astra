#pragma once
#include <string>
#include <optional>
#include <vector>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/builder/basic/document.hpp>

namespace mongoclient {

class IMongoClient {
public:
    virtual ~IMongoClient() = default;
    virtual void connect(const std::string& uri) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    [[nodiscard]] virtual std::optional<bsoncxx::document::value> findOne(
        const std::string& database, 
        const std::string& collection,
        const bsoncxx::document::view& query) = 0;

    virtual void insertOne(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& document) = 0;

    virtual void insertMany(
        const std::string& database,
        const std::string& collection,
        const std::vector<bsoncxx::document::value>& documents) = 0;

    virtual void updateMany(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter,
        const bsoncxx::document::view& update) = 0;

    virtual void deleteMany(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter) = 0;

    [[nodiscard]] virtual std::vector<bsoncxx::document::value> find(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& query) = 0;
};

} // namespace mongoclient