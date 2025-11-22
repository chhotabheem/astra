#include "MongoClient.h"
#include <Logger.h>
#include <stdexcept>

namespace mongoclient {
    MongoClient::MongoClient() : client_(nullptr) {
        // The mongocxx::instance constructor and destructor initialize and shut down the driver,
        // respectively. Therefore, a mongocxx::instance must be created before using the driver and
        // must remain alive for as long as the driver is in use.
        static mongocxx::instance instance{};
        LOG_DEBUG("MongoClient instance created");
    }
    
    MongoClient::~MongoClient() {
        disconnect();
        LOG_DEBUG("MongoClient instance destroyed");
    }
    
    void MongoClient::connect(const std::string& uri) {
        if (isConnected()) {
            LOG_WARN("Already connected to MongoDB");
            throw std::runtime_error("Already connected to MongoDB");
        }
        
        LOG_INFO("Connecting to MongoDB: " + uri);
        mongocxx::uri mongoUri(uri);
        client_ = std::make_unique<mongocxx::client>(mongoUri);
        LOG_INFO("Successfully connected to MongoDB");
    }
    
    void MongoClient::disconnect() {
        if (isConnected()) {
            LOG_INFO("Disconnecting from MongoDB");
            client_.reset();
            LOG_INFO("Disconnected from MongoDB");
        }
    }
    
    bool MongoClient::isConnected() const {
        return client_ != nullptr;
    }
    
    std::optional<bsoncxx::document::value> MongoClient::findOne(
        const std::string& database, 
        const std::string& collection,
        const bsoncxx::document::view& query) {
        if (!isConnected()) {
            LOG_ERROR("Attempted to query while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }
        
        LOG_DEBUG("Querying database: " + database + ", collection: " + collection);
        auto db = (*client_)[database];
        auto coll = db[collection];
        auto result = coll.find_one(query);
        
        if (result) {
            LOG_DEBUG("Document found");
        } else {
            LOG_DEBUG("No document found");
        }
        
        return result;
    }
} // namespace mongoclient