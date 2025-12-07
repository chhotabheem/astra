#include "MongoClient.h"
#include <obs/Log.h>
#include <stdexcept>

namespace mongoclient {
    MongoClient::MongoClient() : m_client(nullptr) {
        // The mongocxx::instance constructor and destructor initialize and shut down the driver,
        // respectively. Therefore, a mongocxx::instance must be created before using the driver and
        // must remain alive for as long as the driver is in use.
        static mongocxx::instance instance{};
        obs::debug("MongoClient instance created");
    }
    
    MongoClient::~MongoClient() {
        disconnect();
        obs::debug("MongoClient instance destroyed");
    }
    
    void MongoClient::connect(const std::string& uri) {
        if (isConnected()) {
            obs::warn("Already connected to MongoDB");
            throw std::runtime_error("Already connected to MongoDB");
        }
        
        obs::info("Connecting to MongoDB: " + uri);
        mongocxx::uri mongoUri(uri);
        m_client = std::make_unique<mongocxx::client>(mongoUri);
        obs::info("Successfully connected to MongoDB");
    }
    
    void MongoClient::disconnect() {
        if (isConnected()) {
            obs::info("Disconnecting from MongoDB");
            m_client.reset();
            obs::info("Disconnected from MongoDB");
        }
    }
    
    bool MongoClient::isConnected() const {
        return m_client != nullptr;
    }
    
    std::optional<bsoncxx::document::value> MongoClient::findOne(
        const std::string& database, 
        const std::string& collection,
        const bsoncxx::document::view& query) {
        if (!isConnected()) {
            obs::error("Attempted to query while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }
        
        obs::debug("Querying database: " + database + ", collection: " + collection);
        auto db = (*m_client)[database];
        auto coll = db[collection];
        auto result = coll.find_one(query);
        
        if (result) {
            obs::debug("Document found");
        } else {
            obs::debug("No document found");
        }
        
        return result;
    }

    void MongoClient::insertOne(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& document) {
        if (!isConnected()) {
            obs::error("Attempted to insert while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        obs::debug("Inserting into database: " + database + ", collection: " + collection);
        auto db = (*m_client)[database];
        auto coll = db[collection];
        coll.insert_one(document);
        obs::debug("Document inserted");
    }

    void MongoClient::insertMany(
        const std::string& database,
        const std::string& collection,
        const std::vector<bsoncxx::document::value>& documents) {
        if (!isConnected()) {
            obs::error("Attempted to insert many while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        obs::debug("Inserting many into database: " + database + ", collection: " + collection);
        auto db = (*m_client)[database];
        auto coll = db[collection];
        coll.insert_many(documents);
        obs::debug("Documents inserted");
    }

    void MongoClient::updateMany(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter,
        const bsoncxx::document::view& update) {
        if (!isConnected()) {
            obs::error("Attempted to update while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        obs::debug("Updating database: " + database + ", collection: " + collection);
        auto db = (*m_client)[database];
        auto coll = db[collection];
        coll.update_many(filter, update);
        obs::debug("Documents updated");
    }

    void MongoClient::deleteMany(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& filter) {
        if (!isConnected()) {
            obs::error("Attempted to delete while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        obs::debug("Deleting from database: " + database + ", collection: " + collection);
        auto db = (*m_client)[database];
        auto coll = db[collection];
        coll.delete_many(filter);
        obs::debug("Documents deleted");
    }

    std::vector<bsoncxx::document::value> MongoClient::find(
        const std::string& database,
        const std::string& collection,
        const bsoncxx::document::view& query) {
        if (!isConnected()) {
            obs::error("Attempted to find while not connected to MongoDB");
            throw std::runtime_error("Not connected to MongoDB");
        }

        obs::debug("Finding in database: " + database + ", collection: " + collection);
        auto db = (*m_client)[database];
        auto coll = db[collection];
        auto cursor = coll.find(query);

        std::vector<bsoncxx::document::value> results;
        for (auto&& doc : cursor) {
            results.push_back(bsoncxx::document::value(doc));
        }

        obs::debug("Found " + std::to_string(results.size()) + " documents");
        return results;
    }
} // namespace mongoclient

