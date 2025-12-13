#pragma once

#include "Result.h"
#include "DomainErrors.h"
#include <string>
#include <string_view>

namespace url_shortener::domain {

constexpr size_t kMinCodeLength = 6;
constexpr size_t kMaxCodeLength = 8;

class ShortCode {
public:
    using CreateResult = astra::Result<ShortCode, DomainError>;

    [[nodiscard]] static CreateResult create(std::string_view raw);
    [[nodiscard]] static ShortCode from_trusted(std::string raw);
    [[nodiscard]] std::string_view value() const noexcept { return m_value; }

    [[nodiscard]] bool operator==(const ShortCode& other) const noexcept {
        return m_value == other.m_value;
    }

    [[nodiscard]] bool operator!=(const ShortCode& other) const noexcept {
        return !(*this == other);
    }

private:
    explicit ShortCode(std::string value) : m_value(std::move(value)) {}
    std::string m_value;
};

} // namespace url_shortener::domain
