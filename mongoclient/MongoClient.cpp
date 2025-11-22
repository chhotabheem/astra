#include "MongoClient.h"
#include <stdexcept>
namespace mongoclient {
    MongoClient::MongoClient() : client_(nullptr) {
        // The mongocxx::instance constructor and destructor initialize and shut down the driver,
        // respectively. Therefore, a mongocxx::instance must be created before using the driver and
        // must remain alive for as long as the driver is in use.
        static mongocxx::instance instance{};
    }
    MongoClient::~MongoClient() {
        disconnect();
    }
    void MongoClient::connect(const std::string& uri) {
        if (isConnected()) {
            throw std::runtime_error("Already connected to MongoDB");
        }
        mongocxx::uri mongoUri(uri);
        client_ = std::make_unique<mongocxx::client>(mongoUri);
    }
    void MongoClient::disconnect() {
        client_.reset();
    }
    bool MongoClient::isConnected() const {
        return client_ != nullptr;
    }
    std::optional<bsoncxx::document::value> MongoClient::findOne(
        const std::string& database, 
        const std::string& collection,
        const bsoncxx::document::view& query) {
        if (!isConnected()) {
            throw std::runtime_error("Not connected to MongoDB");
        }
        auto db = (*client_)[database];
        auto coll = db[collection];
        return coll.find_one(query);
    }
} // namespace mongoclient