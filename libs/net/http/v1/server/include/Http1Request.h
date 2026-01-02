#pragma once

#include "IRequest.h"

#include <boost/beast/http.hpp>
#include <string>
#include <unordered_map>

namespace astra::http1 {

class Request final : public astra::router::IRequest {
public:
  explicit Request(
      boost::beast::http::request<boost::beast::http::string_body> req);

  [[nodiscard]] const std::string &method() const override;
  [[nodiscard]] const std::string &path() const override;
  [[nodiscard]] std::string header(const std::string &name) const override;
  [[nodiscard]] const std::string &body() const override;
  [[nodiscard]] std::string path_param(const std::string &key) const override;
  [[nodiscard]] std::string query_param(const std::string &key) const override;

  // Internal setter for Router
  void
  set_path_params(std::unordered_map<std::string, std::string> params) override;

private:
  boost::beast::http::request<boost::beast::http::string_body> req_;
  mutable std::string method_str_;
  mutable std::string path_str_;
  mutable std::string body_str_;
  std::unordered_map<std::string, std::string> path_params_;
};

} // namespace astra::http1
