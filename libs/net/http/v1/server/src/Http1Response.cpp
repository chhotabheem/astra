#include "Http1Response.h"

namespace astra::http1 {

Response::Response(SendCallback callback) : callback_(std::move(callback)) {
  res_.version(11); // HTTP/1.1
}

void Response::set_status(int status_code) noexcept {
  res_.result(static_cast<boost::beast::http::status>(status_code));
}

void Response::set_header(const std::string &name, const std::string &value) {
  res_.set(boost::beast::string_view(name.data(), name.size()),
           boost::beast::string_view(value.data(), value.size()));
}

void Response::write(const std::string &content) {
  res_.body().append(content);
}

void Response::close() {
  if (!closed_) {
    res_.prepare_payload();
    if (callback_) {
      callback_(std::move(res_));
    }
    closed_ = true;
  }
}

bool Response::is_alive() const noexcept {
  return !closed_;
}

} // namespace astra::http1
