#pragma once

#include "IResponse.h"

#include <boost/beast/http.hpp>
#include <functional>
#include <memory>

namespace astra::http1 {

class Response final : public astra::router::IResponse {
public:
  using SendCallback = std::function<void(
      boost::beast::http::response<boost::beast::http::string_body>)>;

  explicit Response(SendCallback callback);

  void set_status(int status_code) noexcept override;
  void set_header(const std::string &name, const std::string &value) override;
  void write(const std::string &content) override;
  void close() override;
  [[nodiscard]] bool is_alive() const noexcept override;

private:
  boost::beast::http::response<boost::beast::http::string_body> res_;
  SendCallback callback_;
  bool closed_ = false;
};

} // namespace astra::http1
