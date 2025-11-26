#include "Http1Response.hpp"

namespace http1 {

Response::Response(SendCallback callback)
    : callback_(std::move(callback)) {
    res_.version(11); // HTTP/1.1
}

void Response::set_status(int status_code) noexcept {
    res_.result(static_cast<boost::beast::http::status>(status_code));
}

void Response::set_header(std::string_view name, std::string_view value) {
    res_.set(boost::beast::string_view(name.data(), name.size()), 
             boost::beast::string_view(value.data(), value.size()));
}

void Response::write(std::string_view content) {
    res_.body().append(content.data(), content.size());
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

} // namespace http1
