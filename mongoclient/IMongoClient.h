#pragma once
#include <string>
#include <optional>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/builder/basic/document.hpp>

namespace mongoclient{
    class IMongoClient{
        public:
        virtual ~IMongoClient() = default;
        virtual void connect(const std::string& uri) = 0;
        virtual void disconnect() = 0;
        virtual bool isConnected() const = 0;
        virtual std::optional<bsoncxx::document::value> findOne(
            const std::string& database, 
            const std::string& collection,
            const bsoncxx::document::view& query) = 0;
    };
}