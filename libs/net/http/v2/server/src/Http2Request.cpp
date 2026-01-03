#include "Http2Request.h"

namespace astra::http2 {

Http2Request::Http2Request(
    std::string method, std::string path,
    std::map<std::string, std::string> headers, std::string body,
    std::unordered_map<std::string, std::string> query_params)
    : m_method(std::move(method)), m_path(std::move(path)),
      m_body(std::move(body)), m_headers(std::move(headers)),
      m_query_params(std::move(query_params)) {
}

const std::string &Http2Request::method() const {
  return m_method;
}

const std::string &Http2Request::path() const {
  return m_path;
}

std::string Http2Request::header(const std::string &key) const {
  auto it = m_headers.find(key);
  if (it != m_headers.end()) {
    return it->second;
  }
  return {};
}

const std::string &Http2Request::body() const {
  return m_body;
}

std::string Http2Request::path_param(const std::string &key) const {
  auto it = m_path_params.find(key);
  if (it != m_path_params.end()) {
    return it->second;
  }
  return {};
}

std::string Http2Request::query_param(const std::string &key) const {
  auto it = m_query_params.find(key);
  if (it != m_query_params.end()) {
    return it->second;
  }
  return {};
}

void Http2Request::set_path_params(
    std::unordered_map<std::string, std::string> params) {
  m_path_params = std::move(params);
}

} // namespace astra::http2
