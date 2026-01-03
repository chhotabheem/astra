#include "MongoClient.h"

#include "BsoncxxMongoClient.h"

namespace astra::mongo {

class MongoClient::Impl {
public:
  BsoncxxMongoClient backend;
};

MongoClient::MongoClient() : m_impl(std::make_unique<Impl>()) {
}

MongoClient::~MongoClient() = default;

Result<void, MongoError> MongoClient::connect(const std::string &uri) {
  return m_impl->backend.connect(uri);
}

void MongoClient::disconnect() {
  m_impl->backend.disconnect();
}

bool MongoClient::isConnected() const {
  return m_impl->backend.isConnected();
}

Result<std::optional<std::string>, MongoError>
MongoClient::findOne(const std::string &database, const std::string &collection,
                     const std::string &queryJson) const {
  return m_impl->backend.findOne(database, collection, queryJson);
}

Result<void, MongoError>
MongoClient::insertOne(const std::string &database,
                       const std::string &collection,
                       const std::string &documentJson) {
  return m_impl->backend.insertOne(database, collection, documentJson);
}

Result<void, MongoError>
MongoClient::insertMany(const std::string &database,
                        const std::string &collection,
                        const std::vector<std::string> &documentsJson) {
  return m_impl->backend.insertMany(database, collection, documentsJson);
}

Result<void, MongoError> MongoClient::updateMany(
    const std::string &database, const std::string &collection,
    const std::string &filterJson, const std::string &updateJson) {
  return m_impl->backend.updateMany(database, collection, filterJson,
                                    updateJson);
}

Result<void, MongoError>
MongoClient::deleteMany(const std::string &database,
                        const std::string &collection,
                        const std::string &filterJson) {
  return m_impl->backend.deleteMany(database, collection, filterJson);
}

Result<std::vector<std::string>, MongoError>
MongoClient::find(const std::string &database, const std::string &collection,
                  const std::string &queryJson) const {
  return m_impl->backend.find(database, collection, queryJson);
}

} // namespace astra::mongo
