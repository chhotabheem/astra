#include "Http1Request.h"

namespace http1 {

Request::Request(boost::beast::http::request<boost::beast::http::string_body> req)
    : req_(std::move(req)) {}

std::string_view Request::method() const {
    method_str_ = std::string(boost::beast::http::to_string(req_.method()));
    return method_str_;
}

std::string_view Request::path() const {
    path_str_ = std::string(req_.target());
    return path_str_;
}

std::string_view Request::header(std::string_view name) const {
    auto it = req_.find(boost::beast::string_view(name.data(), name.size()));
    if (it != req_.end()) {
        return std::string_view(it->value().data(), it->value().size());
    }
    return {};
}

std::string_view Request::body() const {
    return req_.body();
}

std::string_view Request::path_param(std::string_view key) const {
    auto it = path_params_.find(std::string(key));
    if (it != path_params_.end()) {
        return it->second;
    }
    return {};
}

std::string_view Request::query_param(std::string_view key) const {
    // TODO: Implement query param parsing
    (void)key;
    return {};
}

void Request::set_path_params(std::unordered_map<std::string, std::string> params) {
    path_params_ = std::move(params);
}

} // namespace http1
