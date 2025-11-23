#include <iostream>

#include "MongoClient.h"
#include <Logger.h>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::basic::kvp;
int main() {
    // Initialize logger
    logger::Logger::initialize();
    logger::Logger::set_level(logger::Level::INFO);
    
    LOG_INFO("MongoDB Client Application started");
    
    try {
        mongoclient::MongoClient client;
        
        // Connect to MongoDB
        client.connect("mongodb://localhost:27017");
        
        // Query a document
        auto query = bsoncxx::builder::stream::document{} 
            << bsoncxx::builder::stream::finalize;
        
        auto result = client.findOne("test", "users", query.view());
        
        if (result) {
            LOG_INFO("Query successful");
            std::cout << bsoncxx::to_json(*result) << std::endl;
        } else {
            LOG_WARN("No documents found in collection");
        }
        
        // Disconnect
        client.disconnect();
        
        LOG_INFO("Application completed successfully");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Application error: " + std::string(e.what()));
        logger::Logger::shutdown();
        return 1;
    }
    
    logger::Logger::shutdown();
    return 0;
}