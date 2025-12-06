/// @file ExpirationPolicy.cpp
/// @brief ExpirationPolicy value object implementation

#include "domain/value_objects/ExpirationPolicy.h"

namespace url_shortener::domain {

ExpirationPolicy::ExpirationPolicy(Type type, std::optional<TimePoint> expires_at, TimePoint created_at)
    : m_type(type)
    , m_expires_at(expires_at)
    , m_created_at(created_at) {}

ExpirationPolicy ExpirationPolicy::never() {
    return ExpirationPolicy(Type::Never, std::nullopt, Clock::now());
}

ExpirationPolicy ExpirationPolicy::after(Duration duration) {
    auto now = Clock::now();
    return ExpirationPolicy(Type::AtTime, now + duration, now);
}

ExpirationPolicy ExpirationPolicy::at(TimePoint time) {
    return ExpirationPolicy(Type::AtTime, time, Clock::now());
}

bool ExpirationPolicy::expires() const noexcept {
    return m_type != Type::Never;
}

bool ExpirationPolicy::has_expired_at(TimePoint now) const noexcept {
    if (m_type == Type::Never) {
        return false;
    }
    return m_expires_at.has_value() && now >= m_expires_at.value();
}

std::optional<ExpirationPolicy::TimePoint> ExpirationPolicy::expires_at() const noexcept {
    return m_expires_at;
}

bool ExpirationPolicy::operator==(const ExpirationPolicy& other) const noexcept {
    if (m_type != other.m_type) {
        return false;
    }
    if (m_type == Type::Never) {
        return true;  // All "never" policies are equal
    }
    return m_expires_at == other.m_expires_at;
}

bool ExpirationPolicy::operator!=(const ExpirationPolicy& other) const noexcept {
    return !(*this == other);
}

} // namespace url_shortener::domain
