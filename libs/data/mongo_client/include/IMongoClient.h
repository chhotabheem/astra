#pragma once
#include <Result.h>
#include <optional>
#include <string>
#include <vector>

namespace astra::mongo {

// Error type for mongo operations
struct MongoError {
  enum class Code {
    NotConnected,
    AlreadyConnected,
    InvalidJson,
    QueryFailed,
    InsertFailed,
    UpdateFailed,
    DeleteFailed,
    ConnectionFailed
  };
  Code code;
  std::string message;
};

using outcome::Result;

class IMongoClient {
public:
  virtual ~IMongoClient() = default;

  virtual Result<void, MongoError> connect(const std::string &uri) = 0;
  virtual void disconnect() = 0;
  [[nodiscard]] virtual bool isConnected() const = 0;

  [[nodiscard]] virtual Result<std::optional<std::string>, MongoError>
  findOne(const std::string &database, const std::string &collection,
          const std::string &queryJson) const = 0;

  [[nodiscard]] virtual Result<std::vector<std::string>, MongoError>
  find(const std::string &database, const std::string &collection,
       const std::string &queryJson) const = 0;

  virtual Result<void, MongoError>
  insertOne(const std::string &database, const std::string &collection,
            const std::string &documentJson) = 0;

  virtual Result<void, MongoError>
  insertMany(const std::string &database, const std::string &collection,
             const std::vector<std::string> &documentsJson) = 0;

  virtual Result<void, MongoError>
  updateMany(const std::string &database, const std::string &collection,
             const std::string &filterJson, const std::string &updateJson) = 0;

  virtual Result<void, MongoError>
  deleteMany(const std::string &database, const std::string &collection,
             const std::string &filterJson) = 0;
};

} // namespace astra::mongo