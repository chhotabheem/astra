#pragma once

#include "IUriService.hpp"
#include "IUriRepository.hpp"
#include <memory>
#include <vector>
#include <cstdint>

namespace uri_shortener {

class UriService : public IUriService {
public:
    explicit UriService(std::shared_ptr<IUriRepository> repository);
    ~UriService() override = default;

    std::string shorten(const std::string& long_url) override;
    std::optional<std::string> expand(const std::string& short_code) override;

private:
    std::shared_ptr<IUriRepository> repository_;
    
    // Base62 Logic
    static std::string encode_base62(uint64_t id);
    static const std::string BASE62_ALPHABET;
};

} // namespace uri_shortener
