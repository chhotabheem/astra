#include <iostream>

#include "MongoClient.h"
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

int main() {
    mongoclient::MongoClient mongoClient;
    mongoClient.connect("mongodb://172.17.0.3:27017");
    
    if (mongoClient.isConnected()) {
        std::cout << "Connected to MongoDB" << std::endl;
        
        // Query MongoDB using MongoClient API
        auto query = make_document(kvp("title", "Understanding MongoDB Basics"));
        auto result = mongoClient.findOne("sample_mflix", "posts", query.view());
        
        if (result) {
            std::cout << bsoncxx::to_json(*result) << std::endl;
        } else {
            std::cout << "No result found" << std::endl;
        }
    }
    
    mongoClient.disconnect();
    return 0;
}