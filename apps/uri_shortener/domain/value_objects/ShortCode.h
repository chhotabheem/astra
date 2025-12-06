/// @file ShortCode.h
/// @brief ShortCode value object - self-validating, immutable

#pragma once

#include "Result.h"
#include "domain/errors/DomainErrors.h"
#include <string>
#include <string_view>

namespace url_shortener::domain {

/// Minimum length for a valid short code
constexpr size_t kMinCodeLength = 6;
/// Maximum length for a valid short code  
constexpr size_t kMaxCodeLength = 8;

/**
 * @brief ShortCode value object
 * 
 * Represents the short code portion of a shortened URL (e.g., "abc123").
 * Self-validating at construction time - invalid codes cannot exist.
 * Immutable after creation.
 * 
 * Invariants:
 * - Length: 6-8 characters
 * - Characters: alphanumeric only (a-z, A-Z, 0-9)
 */
class ShortCode {
public:
    /// Result type for factory methods
    using CreateResult = astra::Result<ShortCode, DomainError>;

    /**
     * @brief Factory method - validates and creates ShortCode
     * @param raw The raw string to validate
     * @return Ok(ShortCode) if valid, Err(DomainError::InvalidShortCode) otherwise
     */
    [[nodiscard]] static CreateResult create(std::string_view raw);

    /**
     * @brief Trusted construction - skips validation
     * @param raw Pre-validated string (e.g., from database)
     * @return ShortCode without validation
     * @warning Only use when loading from trusted sources
     */
    [[nodiscard]] static ShortCode from_trusted(std::string raw);

    /// Get the code value
    [[nodiscard]] std::string_view value() const noexcept { return m_value; }

    /// Equality comparison
    [[nodiscard]] bool operator==(const ShortCode& other) const noexcept {
        return m_value == other.m_value;
    }

    [[nodiscard]] bool operator!=(const ShortCode& other) const noexcept {
        return !(*this == other);
    }

private:
    explicit ShortCode(std::string value) : m_value(std::move(value)) {}

    std::string m_value;
};

} // namespace url_shortener::domain
