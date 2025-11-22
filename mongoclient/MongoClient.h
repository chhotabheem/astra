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
        std::optional<bsoncxx::document::value> findOne(
            const std::string& database, 
            const std::string& collection,
            const bsoncxx::document::view& query) override;
    private:
        std::unique_ptr<mongocxx::client> client_;
    };
} // namespace mongoclient