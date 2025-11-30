#pragma once

#include <string_view>
#include <unordered_map>

namespace router {

class IRequest {
public:
    virtual ~IRequest() = default;

    [[nodiscard]] virtual std::string_view method() const = 0;
    [[nodiscard]] virtual std::string_view path() const = 0;
    [[nodiscard]] virtual std::string_view header(std::string_view key) const = 0;
    [[nodiscard]] virtual std::string_view body() const = 0;

    [[nodiscard]] virtual std::string_view path_param(std::string_view key) const = 0;
    [[nodiscard]] virtual std::string_view query_param(std::string_view key) const = 0;

    virtual void set_path_params(std::unordered_map<std::string_view, std::string_view> params) = 0;
};

} // namespace router
