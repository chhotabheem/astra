#pragma once

#include "DomainErrors.h"
#include "ExpirationPolicy.h"
#include "OriginalUrl.h"
#include "Result.h"
#include "ShortCode.h"

#include <chrono>

namespace uri_shortener::domain {

class ShortLink {
public:
  using Clock = std::chrono::system_clock;
  using TimePoint = Clock::time_point;
  using CreateResult = astra::outcome::Result<ShortLink, DomainError>;

  [[nodiscard]] static CreateResult
  create(ShortCode code, OriginalUrl original,
         ExpirationPolicy expiration = ExpirationPolicy::never());

  [[nodiscard]] const ShortCode &code() const noexcept {
    return m_code;
  }
  [[nodiscard]] const OriginalUrl &original() const noexcept {
    return m_original;
  }
  [[nodiscard]] const ExpirationPolicy &expiration() const noexcept {
    return m_expiration;
  }
  [[nodiscard]] TimePoint created_at() const noexcept {
    return m_created_at;
  }
  [[nodiscard]] bool is_expired() const noexcept;
  [[nodiscard]] bool is_active() const noexcept {
    return !is_expired();
  }

  [[nodiscard]] bool operator==(const ShortLink &other) const noexcept {
    return m_code == other.m_code;
  }

  [[nodiscard]] bool operator!=(const ShortLink &other) const noexcept {
    return !(*this == other);
  }

private:
  ShortLink(ShortCode code, OriginalUrl original, ExpirationPolicy expiration,
            TimePoint created_at);

  ShortCode m_code;
  OriginalUrl m_original;
  ExpirationPolicy m_expiration;
  TimePoint m_created_at;
};

} // namespace uri_shortener::domain
