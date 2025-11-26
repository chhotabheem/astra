#include "Http1Request.hpp"

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

} // namespace http1
