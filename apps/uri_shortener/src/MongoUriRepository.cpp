#include "MongoUriRepository.h"
#include <stdexcept>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

namespace uri_shortener {

const std::string MongoUriRepository::DB_NAME = "uri_shortener";
const std::string MongoUriRepository::COLLECTION_NAME = "urls";

MongoUriRepository::MongoUriRepository(std::shared_ptr<mongoclient::MongoClient> mongo)
    : mongo_(std::move(mongo)) {
    if (!mongo_) {
        throw std::invalid_argument("MongoClient cannot be null");
    }
}

uint64_t MongoUriRepository::generate_id() {
    // MongoDB is not responsible for ID generation in this design (Redis is).
    // This method should not be called on the Mongo repository directly in the current flow.
    // However, to satisfy the interface, we could implement a fallback or throw.
    throw std::runtime_error("MongoUriRepository does not support ID generation");
}

void MongoUriRepository::save(const std::string& short_code, const std::string& long_url) {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    auto doc = document{} 
        << "short_code" << short_code
        << "long_url" << long_url
        << "created_at" << bsoncxx::types::b_date(std::chrono::system_clock::now())
        << finalize;

    mongo_->insertOne(DB_NAME, COLLECTION_NAME, doc.view());
}

std::optional<std::string> MongoUriRepository::find(const std::string& short_code) {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    auto query = document{} << "short_code" << short_code << finalize;
    auto result = mongo_->findOne(DB_NAME, COLLECTION_NAME, query.view());

    if (result) {
        auto view = result->view();
        if (view["long_url"] && view["long_url"].type() == bsoncxx::type::k_string) {
            return std::string(view["long_url"].get_string().value);
        }
    }
    return std::nullopt;
}

} // namespace uri_shortener
