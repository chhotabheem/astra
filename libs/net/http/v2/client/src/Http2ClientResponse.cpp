#include "Http2ClientResponse.h"

namespace astra::http2 {

Http2ClientResponse::Http2ClientResponse(
    int status_code, std::string body,
    std::map<std::string, std::string> headers)
    : m_status_code(status_code), m_body(std::move(body)),
      m_headers(std::move(headers)) {
}

std::string Http2ClientResponse::header(const std::string &name) const {
  auto it = m_headers.find(name);
  if (it != m_headers.end()) {
    return it->second;
  }
  return "";
}

} // namespace astra::http2
