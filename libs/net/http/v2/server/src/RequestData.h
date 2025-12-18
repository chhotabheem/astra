#pragma once

#include <string>
#include <map>
#include <unordered_map>

namespace astra::http2 {

/**
 * @brief Data structure holding actual HTTP request data.
 * 
 * Owned by nghttp2 Context (via shared_ptr).
 * Request holds weak_ptr to this.
 */
struct RequestData {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> path_params;
    std::unordered_map<std::string, std::string> query_params;
};

} // namespace astra::http2
