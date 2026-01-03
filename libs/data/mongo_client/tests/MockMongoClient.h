#pragma once

#include "IMongoClient.h"

#include <gmock/gmock.h>

namespace astra::mongo {

class MockMongoClient : public IMongoClient {
public:
  MOCK_METHOD((Result<void, MongoError>), connect, (const std::string &uri),
              (override));
  MOCK_METHOD(void, disconnect, (), (override));
  MOCK_METHOD(bool, isConnected, (), (const, override));

  MOCK_METHOD((Result<std::optional<std::string>, MongoError>), findOne,
              (const std::string &database, const std::string &collection,
               const std::string &queryJson),
              (const, override));

  MOCK_METHOD((Result<std::vector<std::string>, MongoError>), find,
              (const std::string &database, const std::string &collection,
               const std::string &queryJson),
              (const, override));

  MOCK_METHOD((Result<void, MongoError>), insertOne,
              (const std::string &database, const std::string &collection,
               const std::string &documentJson),
              (override));

  MOCK_METHOD((Result<void, MongoError>), insertMany,
              (const std::string &database, const std::string &collection,
               const std::vector<std::string> &documentsJson),
              (override));

  MOCK_METHOD((Result<void, MongoError>), updateMany,
              (const std::string &database, const std::string &collection,
               const std::string &filterJson, const std::string &updateJson),
              (override));

  MOCK_METHOD((Result<void, MongoError>), deleteMany,
              (const std::string &database, const std::string &collection,
               const std::string &filterJson),
              (override));
};

} // namespace astra::mongo
