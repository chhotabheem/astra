/// @file ShortLink.h
/// @brief ShortLink entity (Aggregate Root)

#pragma once

#include "Result.h"
#include "domain/value_objects/ShortCode.h"
#include "domain/value_objects/OriginalUrl.h"
#include "domain/value_objects/ExpirationPolicy.h"
#include "domain/errors/DomainErrors.h"
#include <chrono>

namespace url_shortener::domain {

/**
 * @brief ShortLink - Aggregate Root
 * 
 * Represents a shortened URL mapping from a ShortCode to an OriginalUrl.
 * Identity is based on the ShortCode (two links are equal if they have the same code).
 * 
 * Invariants:
 * - Must have valid ShortCode
 * - Must have valid OriginalUrl
 * - Has an ExpirationPolicy (defaults to never)
 */
class ShortLink {
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using CreateResult = astra::Result<ShortLink, DomainError>;

    /**
     * @brief Factory method - creates a ShortLink
     * @param code The short code
     * @param original The original URL
     * @param expiration Expiration policy (defaults to never)
     * @return Ok(ShortLink) on success
     */
    [[nodiscard]] static CreateResult create(
        ShortCode code,
        OriginalUrl original,
        ExpirationPolicy expiration = ExpirationPolicy::never()
    );

    /// Get the short code (identity)
    [[nodiscard]] const ShortCode& code() const noexcept { return m_code; }

    /// Get the original URL
    [[nodiscard]] const OriginalUrl& original() const noexcept { return m_original; }

    /// Get the expiration policy
    [[nodiscard]] const ExpirationPolicy& expiration() const noexcept { return m_expiration; }

    /// Get creation timestamp
    [[nodiscard]] TimePoint created_at() const noexcept { return m_created_at; }

    /// Check if the link has expired (at current time)
    [[nodiscard]] bool is_expired() const noexcept;

    /// Check if the link is active (not expired)
    [[nodiscard]] bool is_active() const noexcept { return !is_expired(); }

    /// Equality (based on identity = code)
    [[nodiscard]] bool operator==(const ShortLink& other) const noexcept {
        return m_code == other.m_code;
    }

    [[nodiscard]] bool operator!=(const ShortLink& other) const noexcept {
        return !(*this == other);
    }

private:
    ShortLink(ShortCode code, OriginalUrl original, ExpirationPolicy expiration, TimePoint created_at);

    ShortCode m_code;
    OriginalUrl m_original;
    ExpirationPolicy m_expiration;
    TimePoint m_created_at;
};

} // namespace url_shortener::domain
