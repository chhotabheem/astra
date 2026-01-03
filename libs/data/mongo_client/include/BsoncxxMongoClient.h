#pragma once

#include "IMongoClient.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/document/value.hpp>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <optional>
#include <string>
#include <vector>

namespace astra::mongo {

class BsoncxxMongoClient {
public:
  BsoncxxMongoClient();
  ~BsoncxxMongoClient() = default;

  Result<void, MongoError> connect(const std::string &uri);
  void disconnect();
  [[nodiscard]] bool isConnected() const;

  [[nodiscard]] Result<std::optional<std::string>, MongoError>
  findOne(const std::string &database, const std::string &collection,
          const std::string &queryJson) const;

  [[nodiscard]] Result<std::vector<std::string>, MongoError>
  find(const std::string &database, const std::string &collection,
       const std::string &queryJson) const;

  Result<void, MongoError> insertOne(const std::string &database,
                                     const std::string &collection,
                                     const std::string &documentJson);

  Result<void, MongoError>
  insertMany(const std::string &database, const std::string &collection,
             const std::vector<std::string> &documentsJson);

  Result<void, MongoError> updateMany(const std::string &database,
                                      const std::string &collection,
                                      const std::string &filterJson,
                                      const std::string &updateJson);

  Result<void, MongoError> deleteMany(const std::string &database,
                                      const std::string &collection,
                                      const std::string &filterJson);

private:
  std::unique_ptr<mongocxx::client> m_client;
};

} // namespace astra::mongo
