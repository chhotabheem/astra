/// @file ShortCode.cpp
/// @brief ShortCode value object implementation

#include "ShortCode.h"

#include <algorithm>
#include <cctype>

namespace uri_shortener::domain {

ShortCode::CreateResult ShortCode::create(const std::string &raw) {
  // Validate length
  if (raw.length() < kMinCodeLength || raw.length() > kMaxCodeLength) {
    return CreateResult::Err(DomainError::InvalidShortCode);
  }

  // Validate characters (alphanumeric only)
  bool all_alnum = std::all_of(raw.begin(), raw.end(), [](char c) {
    return std::isalnum(static_cast<unsigned char>(c));
  });

  if (!all_alnum) {
    return CreateResult::Err(DomainError::InvalidShortCode);
  }

  return CreateResult::Ok(ShortCode(raw));
}

ShortCode ShortCode::from_trusted(std::string raw) {
  return ShortCode(std::move(raw));
}

} // namespace uri_shortener::domain
