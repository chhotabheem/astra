/// @file ExpirationPolicy.h
/// @brief ExpirationPolicy value object - defines when a link expires

#pragma once

#include <chrono>
#include <optional>

namespace url_shortener::domain {

/**
 * @brief ExpirationPolicy value object
 * 
 * Defines when (if ever) a shortened link expires.
 * Immutable after creation.
 * 
 * Three types:
 * - Never: Link never expires
 * - AfterDuration: Link expires X time after creation
 * - AtTime: Link expires at a specific timestamp
 */
class ExpirationPolicy {
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    /// Create a policy that never expires
    [[nodiscard]] static ExpirationPolicy never();

    /// Create a policy that expires after a duration from now
    [[nodiscard]] static ExpirationPolicy after(Duration duration);

    /// Create a policy that expires at a specific time
    [[nodiscard]] static ExpirationPolicy at(TimePoint time);

    /// Check if this policy can expire (false for "never" policies)
    [[nodiscard]] bool expires() const noexcept;

    /// Check if the policy has expired at the given time
    [[nodiscard]] bool has_expired_at(TimePoint now) const noexcept;

    /// Get the expiry time (nullopt for "never" policies)
    [[nodiscard]] std::optional<TimePoint> expires_at() const noexcept;

    /// Get when this policy was created (for duration-based policies)
    [[nodiscard]] TimePoint created_at() const noexcept { return m_created_at; }

    /// Equality comparison
    [[nodiscard]] bool operator==(const ExpirationPolicy& other) const noexcept;
    [[nodiscard]] bool operator!=(const ExpirationPolicy& other) const noexcept;

private:
    enum class Type { Never, AtTime };

    ExpirationPolicy(Type type, std::optional<TimePoint> expires_at, TimePoint created_at);

    Type m_type;
    std::optional<TimePoint> m_expires_at;
    TimePoint m_created_at;
};

} // namespace url_shortener::domain
