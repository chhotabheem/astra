#include "Http1Request.h"

namespace astra::http1 {

Request::Request(
    boost::beast::http::request<boost::beast::http::string_body> req)
    : req_(std::move(req)) {
}

const std::string &Request::method() const {
  method_str_ = std::string(boost::beast::http::to_string(req_.method()));
  return method_str_;
}

const std::string &Request::path() const {
  path_str_ = std::string(req_.target());
  return path_str_;
}

std::string Request::header(const std::string &name) const {
  auto it = req_.find(boost::beast::string_view(name.data(), name.size()));
  if (it != req_.end()) {
    return std::string(it->value().data(), it->value().size());
  }
  return {};
}

const std::string &Request::body() const {
  body_str_ = req_.body();
  return body_str_;
}

std::string Request::path_param(const std::string &key) const {
  auto it = path_params_.find(key);
  if (it != path_params_.end()) {
    return it->second;
  }
  return {};
}

std::string Request::query_param(const std::string &key) const {
  // TODO: Implement query param parsing
  (void)key;
  return {};
}

void Request::set_path_params(
    std::unordered_map<std::string, std::string> params) {
  path_params_ = std::move(params);
}

} // namespace astra::http1
