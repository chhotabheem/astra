#pragma once

#include "http_abstractions/IRequest.hpp"
#include <boost/beast/http.hpp>
#include <string>

namespace http1 {

class Request : public http_abstractions::IRequest {
public:
    explicit Request(boost::beast::http::request<boost::beast::http::string_body> req);

    [[nodiscard]] std::string_view method() const override;
    [[nodiscard]] std::string_view path() const override;
    [[nodiscard]] std::string_view header(std::string_view name) const override;
    [[nodiscard]] std::string_view body() const override;

private:
    boost::beast::http::request<boost::beast::http::string_body> req_;
    mutable std::string method_str_; // Cache for string_view if needed
    mutable std::string path_str_;
};

} // namespace http1
