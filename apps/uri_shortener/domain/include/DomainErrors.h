#pragma once

#include <string>

namespace uri_shortener::domain {

enum class DomainError {
  InvalidShortCode,
  InvalidUrl,

  LinkNotFound,
  LinkExpired,
  LinkAlreadyExists,

  CodeGenerationFailed
};

inline std::string to_string(DomainError error) noexcept {
  switch (error) {
  case DomainError::InvalidShortCode:
    return "Invalid short code format";
  case DomainError::InvalidUrl:
    return "Invalid URL format";
  case DomainError::LinkNotFound:
    return "Link not found";
  case DomainError::LinkExpired:
    return "Link has expired";
  case DomainError::LinkAlreadyExists:
    return "Link already exists";
  case DomainError::CodeGenerationFailed:
    return "Code generation failed";
  default:
    return "Unknown error";
  }
}

} // namespace uri_shortener::domain
