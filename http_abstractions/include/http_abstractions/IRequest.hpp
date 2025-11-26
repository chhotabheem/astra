#pragma once

#include <string_view>

namespace http_abstractions {

class IRequest {
public:
    virtual ~IRequest() = default;

    [[nodiscard]] virtual std::string_view method() const = 0;
    [[nodiscard]] virtual std::string_view path() const = 0;
    [[nodiscard]] virtual std::string_view header(std::string_view key) const = 0;
    [[nodiscard]] virtual std::string_view body() const = 0;
};

} // namespace http_abstractions
