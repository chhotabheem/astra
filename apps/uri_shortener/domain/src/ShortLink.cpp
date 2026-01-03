/// @file ShortLink.cpp
/// @brief ShortLink entity implementation

#include "ShortLink.h"

namespace uri_shortener::domain {

ShortLink::ShortLink(ShortCode code, OriginalUrl original,
                     ExpirationPolicy expiration, TimePoint created_at)
    : m_code(std::move(code)), m_original(std::move(original)),
      m_expiration(std::move(expiration)), m_created_at(created_at) {
}

ShortLink::CreateResult ShortLink::create(ShortCode code, OriginalUrl original,
                                          ExpirationPolicy expiration) {
  return CreateResult::Ok(ShortLink(std::move(code), std::move(original),
                                    std::move(expiration), Clock::now()));
}

bool ShortLink::is_expired() const noexcept {
  return m_expiration.has_expired_at(Clock::now());
}

} // namespace uri_shortener::domain
