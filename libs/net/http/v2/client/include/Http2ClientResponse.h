#pragma once

#include <map>
#include <string>

namespace astra::http2 {

class Http2ClientResponse {
public:
  Http2ClientResponse() = default;
  Http2ClientResponse(int status_code, std::string body,
                      std::map<std::string, std::string> headers);

  [[nodiscard]] int status_code() const {
    return m_status_code;
  }
  [[nodiscard]] const std::string &body() const {
    return m_body;
  }
  [[nodiscard]] std::string header(const std::string &name) const;
  [[nodiscard]] const std::map<std::string, std::string> &headers() const {
    return m_headers;
  }

private:
  int m_status_code = 0;
  std::string m_body;
  std::map<std::string, std::string> m_headers;
};

} // namespace astra::http2
