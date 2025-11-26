#pragma once

#include <string>
#include <map>
#include "Http2Request.hpp"

namespace http2server {

class Request::Impl {
public:
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;

    Impl() = default;
};

} // namespace http2server
