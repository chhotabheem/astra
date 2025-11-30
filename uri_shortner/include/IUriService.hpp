#pragma once

#include <string>
#include <optional>

namespace uri_shortener {

class IUriService {
public:
    virtual ~IUriService() = default;

    /**
     * @brief Shorten a long URL
     * @param long_url The original URL
     * @return The generated short code
     */
    virtual std::string shorten(const std::string& long_url) = 0;

    /**
     * @brief Expand a short code
     * @param short_code The short code
     * @return The original URL if found, std::nullopt otherwise
     */
    virtual std::optional<std::string> expand(const std::string& short_code) = 0;
};

} // namespace uri_shortener
