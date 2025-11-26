#pragma once

#include "http_abstractions/IResponse.hpp"
#include <boost/beast/http.hpp>
#include <functional>
#include <memory>

namespace http1 {

class Response : public http_abstractions::IResponse {
public:
    using SendCallback = std::function<void(boost::beast::http::response<boost::beast::http::string_body>)>;

    explicit Response(SendCallback callback);

    void set_status(int status_code) noexcept override;
    void set_header(std::string_view name, std::string_view value) override;
    void write(std::string_view content) override;
    void close() override;

private:
    boost::beast::http::response<boost::beast::http::string_body> res_;
    SendCallback callback_;
    bool closed_ = false;
};

} // namespace http1
