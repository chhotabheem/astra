#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <string_view>
#include "Http2Request.h"

namespace http2server {

class Request::Impl {
public:
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::unordered_map<std::string_view, std::string_view> path_params;

    Impl() = default;
};

} // namespace http2server
