#pragma once

#include "IUriRepository.h"
#include "RedisClient.h"
#include <memory>

namespace uri_shortener {

class RedisUriRepository : public IUriRepository {
public:
    explicit RedisUriRepository(std::shared_ptr<redisclient::RedisClient> redis);
    ~RedisUriRepository() override = default;

    uint64_t generate_id() override;
    void save(const std::string& short_code, const std::string& long_url) override;
    std::optional<std::string> find(const std::string& short_code) override;

private:
    std::shared_ptr<redisclient::RedisClient> redis_;
    static const std::string ID_KEY;
    static const std::string URL_PREFIX;
};

} // namespace uri_shortener
