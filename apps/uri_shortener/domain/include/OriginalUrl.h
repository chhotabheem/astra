#pragma once

#include "Result.h"
#include "DomainErrors.h"
#include <string>
#include <string_view>

namespace url_shortener::domain {

class OriginalUrl {
public:
    using CreateResult = astra::outcome::Result<OriginalUrl, DomainError>;

    [[nodiscard]] static CreateResult create(std::string_view raw);
    [[nodiscard]] static OriginalUrl from_trusted(std::string raw);
    [[nodiscard]] std::string_view value() const noexcept { return m_value; }

    [[nodiscard]] bool operator==(const OriginalUrl& other) const noexcept {
        return m_value == other.m_value;
    }

    [[nodiscard]] bool operator!=(const OriginalUrl& other) const noexcept {
        return !(*this == other);
    }

private:
    explicit OriginalUrl(std::string value) : m_value(std::move(value)) {}
    std::string m_value;
};

} // namespace url_shortener::domain
