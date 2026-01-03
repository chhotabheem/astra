#pragma once

#include "IMongoClient.h"

#include <memory>

namespace astra::mongo {

class MongoClient : public IMongoClient {
public:
  MongoClient();
  ~MongoClient() override;

  Result<void, MongoError> connect(const std::string &uri) override;
  void disconnect() override;
  [[nodiscard]] bool isConnected() const override;

  [[nodiscard]] Result<std::optional<std::string>, MongoError>
  findOne(const std::string &database, const std::string &collection,
          const std::string &queryJson) const override;

  [[nodiscard]] Result<std::vector<std::string>, MongoError>
  find(const std::string &database, const std::string &collection,
       const std::string &queryJson) const override;

  Result<void, MongoError> insertOne(const std::string &database,
                                     const std::string &collection,
                                     const std::string &documentJson) override;

  Result<void, MongoError>
  insertMany(const std::string &database, const std::string &collection,
             const std::vector<std::string> &documentsJson) override;

  Result<void, MongoError> updateMany(const std::string &database,
                                      const std::string &collection,
                                      const std::string &filterJson,
                                      const std::string &updateJson) override;

  Result<void, MongoError> deleteMany(const std::string &database,
                                      const std::string &collection,
                                      const std::string &filterJson) override;

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace astra::mongo