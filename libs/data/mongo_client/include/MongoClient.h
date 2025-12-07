#pragma once
#include "IMongoClient.h"
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <memory>

namespace mongoclient {

class MongoClient : public IMongoClient {
public:
    MongoClient();
    ~MongoClient() override;
    void connect(const std::string& uri) override;
    void disconnect() override;
    bool isConnected() const override;
    
    [[nodiscard]] std::optional<bsoncxx::document::value> findOne(
        const std::string& database, 
        const std::string& collection,
        const bsoncxx::document::view& query) override;

    void insertOne(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& document) override;

    void insertMany(
        const std::string& database,
        const std::string& collection,
        const std::vector<bsoncxx::document::value>& documents) override;

    void updateMany(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter,
        const bsoncxx::document::view& update) override;

    void deleteMany(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter) override;

    [[nodiscard]] std::vector<bsoncxx::document::value> find(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& query) override;

private:
    std::unique_ptr<mongocxx::client> m_client;
};

} // namespace mongoclient