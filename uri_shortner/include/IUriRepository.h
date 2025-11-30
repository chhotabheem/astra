#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace uri_shortener {

class IUriRepository {
public:
    virtual ~IUriRepository() = default;

    /**
     * @brief Generate a unique ID for a new URL
     * @return A unique 64-bit integer
     */
    virtual uint64_t generate_id() = 0;

    /**
     * @brief Save a mapping from short code to long URL
     * @param short_code The generated short code
     * @param long_url The original URL
     */
    virtual void save(const std::string& short_code, const std::string& long_url) = 0;

    /**
     * @brief Find a long URL by its short code
     * @param short_code The short code
     * @return The original URL if found, std::nullopt otherwise
     */
    virtual std::optional<std::string> find(const std::string& short_code) = 0;
};

} // namespace uri_shortener
