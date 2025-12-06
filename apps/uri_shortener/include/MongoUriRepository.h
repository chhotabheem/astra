#pragma once

#include "IUriRepository.h"
#include "MongoClient.h"
#include <memory>

namespace uri_shortener {

class MongoUriRepository : public IUriRepository {
public:
    explicit MongoUriRepository(std::shared_ptr<mongoclient::MongoClient> mongo);
    ~MongoUriRepository() override = default;

    uint64_t generate_id() override;
    void save(const std::string& short_code, const std::string& long_url) override;
    std::optional<std::string> find(const std::string& short_code) override;

private:
    std::shared_ptr<mongoclient::MongoClient> mongo_;
    static const std::string DB_NAME;
    static const std::string COLLECTION_NAME;
};

} // namespace uri_shortener
