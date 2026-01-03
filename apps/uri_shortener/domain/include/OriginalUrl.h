#pragma once

#include "DomainErrors.h"
#include "Result.h"

#include <string>

namespace uri_shortener::domain {

class OriginalUrl {
public:
  using CreateResult = astra::outcome::Result<OriginalUrl, DomainError>;

  [[nodiscard]] static CreateResult create(const std::string &raw);
  [[nodiscard]] static OriginalUrl from_trusted(std::string raw);
  [[nodiscard]] const std::string &value() const noexcept {
    return m_value;
  }

  [[nodiscard]] bool operator==(const OriginalUrl &other) const noexcept {
    return m_value == other.m_value;
  }

  [[nodiscard]] bool operator!=(const OriginalUrl &other) const noexcept {
    return !(*this == other);
  }

private:
  explicit OriginalUrl(std::string value) : m_value(std::move(value)) {
  }
  std::string m_value;
};

} // namespace uri_shortener::domain
