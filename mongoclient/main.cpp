#include <iostream>

#include "MongoClient.h"
#include <obs/Log.h>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::basic::kvp;
int main() {
    obs::info("MongoDB Client Application started");
    
    try {
        mongoclient::MongoClient client;
        
        // Connect to MongoDB
        client.connect("mongodb://localhost:27017");
        
        // Query a document
        auto query = bsoncxx::builder::stream::document{} 
            << bsoncxx::builder::stream::finalize;
        
        auto result = client.findOne("test", "users", query.view());
        
        if (result) {
            obs::info("Query successful");
            std::cout << bsoncxx::to_json(*result) << std::endl;
        } else {
            obs::warn("No documents found in collection");
        }
        
        // Disconnect
        client.disconnect();
        
        obs::info("Application completed successfully");
        
    } catch (const std::exception& e) {
        obs::error("Application error: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}