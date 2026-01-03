/// @file OriginalUrl.cpp
/// @brief OriginalUrl value object implementation

#include "OriginalUrl.h"

#include <algorithm>

namespace uri_shortener::domain {

namespace {

/// C++17 compatible starts_with helper
bool starts_with(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

/// Check if URL starts with valid scheme
bool has_valid_scheme(const std::string &url) {
  return starts_with(url, "http://") || starts_with(url, "https://");
}

/// Get the part after the scheme
std::string get_authority_and_path(const std::string &url) {
  if (starts_with(url, "https://")) {
    return url.substr(8);
  }
  if (starts_with(url, "http://")) {
    return url.substr(7);
  }
  return {};
}

/// Check for invalid URL characters
bool has_invalid_characters(const std::string &url) {
  // Basic check for obviously invalid characters
  const std::string invalid_chars = " <>\"{}|\\^`";
  return std::any_of(url.begin(), url.end(), [&](char c) {
    return invalid_chars.find(c) != std::string::npos;
  });
}

/// Check if authority (host) portion is valid
bool has_valid_authority(const std::string &authority_and_path) {
  if (authority_and_path.empty()) {
    return false;
  }

  // Extract authority (before first /, ?, or #)
  auto end_of_authority = authority_and_path.find_first_of("/?#");
  std::string authority = (end_of_authority == std::string::npos)
                              ? authority_and_path
                              : authority_and_path.substr(0, end_of_authority);

  // Authority must have at least one character (the host)
  if (authority.empty()) {
    return false;
  }

  // Strip port if present
  auto port_pos = authority.rfind(':');
  if (port_pos != std::string::npos) {
    authority = authority.substr(0, port_pos);
  }

  // Host must have at least one character
  return !authority.empty();
}

} // namespace

OriginalUrl::CreateResult OriginalUrl::create(const std::string &raw) {
  // Check not empty
  if (raw.empty()) {
    return CreateResult::Err(DomainError::InvalidUrl);
  }

  // Check valid scheme
  if (!has_valid_scheme(raw)) {
    return CreateResult::Err(DomainError::InvalidUrl);
  }

  // Check for invalid characters
  if (has_invalid_characters(raw)) {
    return CreateResult::Err(DomainError::InvalidUrl);
  }

  // Check valid authority
  auto authority_and_path = get_authority_and_path(raw);
  if (!has_valid_authority(authority_and_path)) {
    return CreateResult::Err(DomainError::InvalidUrl);
  }

  return CreateResult::Ok(OriginalUrl(raw));
}

OriginalUrl OriginalUrl::from_trusted(std::string raw) {
  return OriginalUrl(std::move(raw));
}

} // namespace uri_shortener::domain
