#pragma once

#include <chrono>
#include <optional>

namespace url_shortener::domain {

class ExpirationPolicy {
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    [[nodiscard]] static ExpirationPolicy never();
    [[nodiscard]] static ExpirationPolicy after(Duration duration);
    [[nodiscard]] static ExpirationPolicy at(TimePoint time);

    [[nodiscard]] bool expires() const noexcept;
    [[nodiscard]] bool has_expired_at(TimePoint now) const noexcept;
    [[nodiscard]] std::optional<TimePoint> expires_at() const noexcept;
    [[nodiscard]] TimePoint created_at() const noexcept { return m_created_at; }

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
