#include "BsoncxxMongoClient.h"

#include <Log.h>
#include <bsoncxx/json.hpp>

namespace astra::mongo {

BsoncxxMongoClient::BsoncxxMongoClient() : m_client(nullptr) {
  static mongocxx::instance instance{};
  obs::debug("BsoncxxMongoClient instance created");
}

Result<void, MongoError> BsoncxxMongoClient::connect(const std::string &uri) {
  if (isConnected()) {
    obs::warn("Already connected to MongoDB");
    return Result<void, MongoError>::Err(MongoError{
        MongoError::Code::AlreadyConnected, "Already connected to MongoDB"});
  }

  try {
    obs::info("Connecting to MongoDB: " + uri);
    mongocxx::uri mongoUri(uri);
    m_client = std::make_unique<mongocxx::client>(mongoUri);
    obs::info("Successfully connected to MongoDB");
    return Result<void, MongoError>::Ok();
  } catch (const std::exception &e) {
    obs::error("Failed to connect to MongoDB: " + std::string(e.what()));
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::ConnectionFailed, e.what()});
  }
}

void BsoncxxMongoClient::disconnect() {
  if (isConnected()) {
    obs::info("Disconnecting from MongoDB");
    m_client.reset();
    obs::info("Disconnected from MongoDB");
  }
}

bool BsoncxxMongoClient::isConnected() const {
  return m_client != nullptr;
}

Result<std::optional<std::string>, MongoError>
BsoncxxMongoClient::findOne(const std::string &database,
                            const std::string &collection,
                            const std::string &queryJson) const {
  if (!isConnected()) {
    obs::error("Attempted to query while not connected to MongoDB");
    return Result<std::optional<std::string>, MongoError>::Err(
        MongoError{MongoError::Code::NotConnected, "Not connected to MongoDB"});
  }

  try {
    obs::debug("Querying database: " + database +
               ", collection: " + collection);
    auto bsonQuery = bsoncxx::from_json(queryJson);
    auto db = (*m_client)[database];
    auto coll = db[collection];
    auto result = coll.find_one(bsonQuery.view());

    if (result) {
      obs::debug("Document found");
      return Result<std::optional<std::string>, MongoError>::Ok(
          bsoncxx::to_json(*result));
    } else {
      obs::debug("No document found");
      return Result<std::optional<std::string>, MongoError>::Ok(std::nullopt);
    }
  } catch (const std::exception &e) {
    obs::error("Query failed: " + std::string(e.what()));
    return Result<std::optional<std::string>, MongoError>::Err(
        MongoError{MongoError::Code::QueryFailed, e.what()});
  }
}

Result<void, MongoError>
BsoncxxMongoClient::insertOne(const std::string &database,
                              const std::string &collection,
                              const std::string &documentJson) {
  if (!isConnected()) {
    obs::error("Attempted to insert while not connected to MongoDB");
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::NotConnected, "Not connected to MongoDB"});
  }

  try {
    obs::debug("Inserting into database: " + database +
               ", collection: " + collection);
    auto bsonDoc = bsoncxx::from_json(documentJson);
    auto db = (*m_client)[database];
    auto coll = db[collection];
    coll.insert_one(bsonDoc.view());
    obs::debug("Document inserted");
    return Result<void, MongoError>::Ok();
  } catch (const std::exception &e) {
    obs::error("Insert failed: " + std::string(e.what()));
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::InsertFailed, e.what()});
  }
}

Result<void, MongoError>
BsoncxxMongoClient::insertMany(const std::string &database,
                               const std::string &collection,
                               const std::vector<std::string> &documentsJson) {
  if (!isConnected()) {
    obs::error("Attempted to insert many while not connected to MongoDB");
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::NotConnected, "Not connected to MongoDB"});
  }

  try {
    obs::debug("Inserting many into database: " + database +
               ", collection: " + collection);
    std::vector<bsoncxx::document::value> bsonDocs;
    bsonDocs.reserve(documentsJson.size());
    for (const auto &json : documentsJson) {
      bsonDocs.push_back(bsoncxx::from_json(json));
    }

    auto db = (*m_client)[database];
    auto coll = db[collection];
    coll.insert_many(bsonDocs);
    obs::debug("Documents inserted");
    return Result<void, MongoError>::Ok();
  } catch (const std::exception &e) {
    obs::error("Insert many failed: " + std::string(e.what()));
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::InsertFailed, e.what()});
  }
}

Result<void, MongoError> BsoncxxMongoClient::updateMany(
    const std::string &database, const std::string &collection,
    const std::string &filterJson, const std::string &updateJson) {
  if (!isConnected()) {
    obs::error("Attempted to update while not connected to MongoDB");
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::NotConnected, "Not connected to MongoDB"});
  }

  try {
    obs::debug("Updating database: " + database +
               ", collection: " + collection);
    auto bsonFilter = bsoncxx::from_json(filterJson);
    auto bsonUpdate = bsoncxx::from_json(updateJson);
    auto db = (*m_client)[database];
    auto coll = db[collection];
    coll.update_many(bsonFilter.view(), bsonUpdate.view());
    obs::debug("Documents updated");
    return Result<void, MongoError>::Ok();
  } catch (const std::exception &e) {
    obs::error("Update failed: " + std::string(e.what()));
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::UpdateFailed, e.what()});
  }
}

Result<void, MongoError>
BsoncxxMongoClient::deleteMany(const std::string &database,
                               const std::string &collection,
                               const std::string &filterJson) {
  if (!isConnected()) {
    obs::error("Attempted to delete while not connected to MongoDB");
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::NotConnected, "Not connected to MongoDB"});
  }

  try {
    obs::debug("Deleting from database: " + database +
               ", collection: " + collection);
    auto bsonFilter = bsoncxx::from_json(filterJson);
    auto db = (*m_client)[database];
    auto coll = db[collection];
    coll.delete_many(bsonFilter.view());
    obs::debug("Documents deleted");
    return Result<void, MongoError>::Ok();
  } catch (const std::exception &e) {
    obs::error("Delete failed: " + std::string(e.what()));
    return Result<void, MongoError>::Err(
        MongoError{MongoError::Code::DeleteFailed, e.what()});
  }
}

Result<std::vector<std::string>, MongoError>
BsoncxxMongoClient::find(const std::string &database,
                         const std::string &collection,
                         const std::string &queryJson) const {
  if (!isConnected()) {
    obs::error("Attempted to find while not connected to MongoDB");
    return Result<std::vector<std::string>, MongoError>::Err(
        MongoError{MongoError::Code::NotConnected, "Not connected to MongoDB"});
  }

  try {
    obs::debug("Finding in database: " + database +
               ", collection: " + collection);
    auto bsonQuery = bsoncxx::from_json(queryJson);
    auto db = (*m_client)[database];
    auto coll = db[collection];
    auto cursor = coll.find(bsonQuery.view());

    std::vector<std::string> results;
    for (auto &&doc : cursor) {
      results.push_back(bsoncxx::to_json(doc));
    }

    obs::debug("Found " + std::to_string(results.size()) + " documents");
    return Result<std::vector<std::string>, MongoError>::Ok(std::move(results));
  } catch (const std::exception &e) {
    obs::error("Find failed: " + std::string(e.what()));
    return Result<std::vector<std::string>, MongoError>::Err(
        MongoError{MongoError::Code::QueryFailed, e.what()});
  }
}

} // namespace astra::mongo
