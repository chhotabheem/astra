#pragma once

#include <string_view>

namespace http_abstractions {

class IResponse {
public:
    virtual ~IResponse() = default;

    virtual void set_status(int code) noexcept = 0;
    virtual void set_header(std::string_view key, std::string_view value) = 0;
    virtual void write(std::string_view data) = 0;
    virtual void close() = 0;
};

} // namespace http_abstractions
