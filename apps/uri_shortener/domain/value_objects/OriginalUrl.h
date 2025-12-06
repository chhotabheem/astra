/// @file OriginalUrl.h
/// @brief OriginalUrl value object - self-validating, immutable

#pragma once

#include "Result.h"
#include "domain/errors/DomainErrors.h"
#include <string>
#include <string_view>

namespace url_shortener::domain {

/**
 * @brief OriginalUrl value object
 * 
 * Represents the original (long) URL to be shortened.
 * Self-validating at construction time - invalid URLs cannot exist.
 * Immutable after creation.
 * 
 * Invariants:
 * - Must have http:// or https:// scheme
 * - Must have a valid host
 * - No invalid characters
 */
class OriginalUrl {
public:
    /// Result type for factory methods
    using CreateResult = astra::Result<OriginalUrl, DomainError>;

    /**
     * @brief Factory method - validates and creates OriginalUrl
     * @param raw The raw URL string to validate
     * @return Ok(OriginalUrl) if valid, Err(DomainError::InvalidUrl) otherwise
     */
    [[nodiscard]] static CreateResult create(std::string_view raw);

    /**
     * @brief Trusted construction - skips validation
     * @param raw Pre-validated URL (e.g., from database)
     * @return OriginalUrl without validation
     * @warning Only use when loading from trusted sources
     */
    [[nodiscard]] static OriginalUrl from_trusted(std::string raw);

    /// Get the URL value
    [[nodiscard]] std::string_view value() const noexcept { return m_value; }

    /// Equality comparison
    [[nodiscard]] bool operator==(const OriginalUrl& other) const noexcept {
        return m_value == other.m_value;
    }

    [[nodiscard]] bool operator!=(const OriginalUrl& other) const noexcept {
        return !(*this == other);
    }

private:
    explicit OriginalUrl(std::string value) : m_value(std::move(value)) {}

    std::string m_value;
};

} // namespace url_shortener::domain
