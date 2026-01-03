#pragma once

#include "IRequest.h"

#include <map>
#include <string>
#include <unordered_map>

namespace astra::http2 {

class Http2Request final : public astra::router::IRequest {
public:
  Http2Request() = default;
  Http2Request(std::string method, std::string path,
               std::map<std::string, std::string> headers = {},
               std::string body = {},
               std::unordered_map<std::string, std::string> query_params = {});

  Http2Request(const Http2Request &) = default;
  Http2Request &operator=(const Http2Request &) = default;
  Http2Request(Http2Request &&) noexcept = default;
  Http2Request &operator=(Http2Request &&) noexcept = default;

  ~Http2Request() override = default;

  [[nodiscard]] const std::string &method() const override;
  [[nodiscard]] const std::string &path() const override;
  [[nodiscard]] std::string header(const std::string &key) const override;
  [[nodiscard]] const std::string &body() const override;

  [[nodiscard]] std::string path_param(const std::string &key) const override;
  [[nodiscard]] std::string query_param(const std::string &key) const override;

  void
  set_path_params(std::unordered_map<std::string, std::string> params) override;

private:
  std::string m_method;
  std::string m_path;
  std::string m_body;
  std::map<std::string, std::string> m_headers;
  std::unordered_map<std::string, std::string> m_path_params;
  std::unordered_map<std::string, std::string> m_query_params;
};

} // namespace astra::http2
