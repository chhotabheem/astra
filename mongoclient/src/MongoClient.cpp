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

    void MongoClient::insertOne(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& document) {
        if (!isConnected()) {
            LOG_ERROR("Attempted to insert while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        LOG_DEBUG("Inserting into database: " + database + ", collection: " + collection);
        auto db = (*client_)[database];
        auto coll = db[collection];
        coll.insert_one(document);
        LOG_DEBUG("Document inserted");
    }

    void MongoClient::insertMany(
        const std::string& database,
        const std::string& collection,
        const std::vector<bsoncxx::document::value>& documents) {
        if (!isConnected()) {
            LOG_ERROR("Attempted to insert many while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        LOG_DEBUG("Inserting many into database: " + database + ", collection: " + collection);
        auto db = (*client_)[database];
        auto coll = db[collection];
        coll.insert_many(documents);
        LOG_DEBUG("Documents inserted");
    }

    void MongoClient::updateMany(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter,
        const bsoncxx::document::view& update) {
        if (!isConnected()) {
            LOG_ERROR("Attempted to update while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        LOG_DEBUG("Updating database: " + database + ", collection: " + collection);
        auto db = (*client_)[database];
        auto coll = db[collection];
        coll.update_many(filter, update);
        LOG_DEBUG("Documents updated");
    }

    void MongoClient::deleteMany(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter) {
        if (!isConnected()) {
            LOG_ERROR("Attempted to delete while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        LOG_DEBUG("Deleting from database: " + database + ", collection: " + collection);
        auto db = (*client_)[database];
        auto coll = db[collection];
        coll.delete_many(filter);
        LOG_DEBUG("Documents deleted");
    }

    std::vector<bsoncxx::document::value> MongoClient::find(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& query) {
        if (!isConnected()) {
            LOG_ERROR("Attempted to find while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        LOG_DEBUG("Finding in database: " + database + ", collection: " + collection);
        auto db = (*client_)[database];
        auto coll = db[collection];
        auto cursor = coll.find(query);

        std::vector<bsoncxx::document::value> results;
        for (auto&& doc : cursor) {
            results.push_back(bsoncxx::document::value(doc));
        }

        LOG_DEBUG("Found " + std::to_string(results.size()) + " documents");
        return results;
    }
} // namespace mongoclient
