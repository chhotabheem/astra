#pragma once

#include <string>
#include <string_view>
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
    [[nodiscard]] virtual std::string shorten(std::string_view long_url) = 0;

    /**
     * @brief Expand a short code
     * @param short_code The short code
     * @return The original URL if found, std::nullopt otherwise
     */
    [[nodiscard]] virtual std::optional<std::string> expand(std::string_view short_code) = 0;
};

} // namespace uri_shortener
