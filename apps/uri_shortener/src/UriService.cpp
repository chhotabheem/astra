#include "UriService.h"
#include <algorithm>
#include <stdexcept>

namespace uri_shortener {

const std::string UriService::BASE62_ALPHABET = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

UriService::UriService(std::shared_ptr<IUriRepository> repository)
    : repository_(std::move(repository)) {
    if (!repository_) {
        throw std::invalid_argument("Repository cannot be null");
    }
}

std::string UriService::shorten(std::string_view long_url) {
    // 1. Generate unique ID
    uint64_t id = repository_->generate_id();

    // 2. Encode to Base62
    std::string short_code = encode_base62(id);

    // 3. Save mapping
    repository_->save(short_code, std::string(long_url));

    return short_code;
}

std::optional<std::string> UriService::expand(std::string_view short_code) {
    return repository_->find(std::string(short_code));
}

std::string UriService::encode_base62(uint64_t id) {
    if (id == 0) return "0";

    std::string result;
    while (id > 0) {
        result += BASE62_ALPHABET[id % 62];
        id /= 62;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

} // namespace uri_shortener
