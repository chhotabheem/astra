#pragma once

#include "IRequest.h"
#include <boost/beast/http.hpp>
#include <string>
#include <unordered_map>

namespace http1 {

class Request final : public router::IRequest {
public:
    explicit Request(boost::beast::http::request<boost::beast::http::string_body> req);

    [[nodiscard]] std::string_view method() const override;
    [[nodiscard]] std::string_view path() const override;
    [[nodiscard]] std::string_view header(std::string_view name) const override;
    [[nodiscard]] std::string_view body() const override;
    [[nodiscard]] std::string_view path_param(std::string_view key) const override;
    [[nodiscard]] std::string_view query_param(std::string_view key) const override;

    // Internal setter for Router
    void set_path_params(std::unordered_map<std::string, std::string> params) override;

private:
    boost::beast::http::request<boost::beast::http::string_body> req_;
    mutable std::string method_str_; // Cache for string_view if needed
    mutable std::string path_str_;
    std::unordered_map<std::string, std::string> path_params_;
};

} // namespace http1
