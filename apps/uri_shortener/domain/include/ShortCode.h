#pragma once

#include "DomainErrors.h"
#include "Result.h"

#include <string>

namespace uri_shortener::domain {

constexpr size_t kMinCodeLength = 6;
constexpr size_t kMaxCodeLength = 8;

class ShortCode {
public:
  using CreateResult = astra::outcome::Result<ShortCode, DomainError>;

  [[nodiscard]] static CreateResult create(const std::string &raw);
  [[nodiscard]] static ShortCode from_trusted(std::string raw);
  [[nodiscard]] const std::string &value() const noexcept {
    return m_value;
  }

  [[nodiscard]] bool operator==(const ShortCode &other) const noexcept {
    return m_value == other.m_value;
  }

  [[nodiscard]] bool operator!=(const ShortCode &other) const noexcept {
    return !(*this == other);
  }

private:
  explicit ShortCode(std::string value) : m_value(std::move(value)) {
  }
  std::string m_value;
};

} // namespace uri_shortener::domain
