#include "Http1Client.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>

namespace http1 {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

Client::Client() {}
Client::~Client() {}

Response Client::get(const std::string& host, const std::string& port, const std::string& target) {
    return perform_request(host, port, target, http::verb::get, "");
}

Response Client::post(const std::string& host, const std::string& port, const std::string& target, const std::string& body) {
    return perform_request(host, port, target, http::verb::post, body);
}

Response Client::perform_request(const std::string& host, const std::string& port, const std::string& target, http::verb method, const std::string& body) {
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        
        if (method == http::verb::post) {
            req.set(http::field::content_type, "text/plain");
            req.body() = body;
            req.prepare_payload();
        }

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        Response response;
        response.status_code = res.result_int();
        response.body = res.body();
        for (auto& field : res) {
            response.headers[std::string(field.name_string())] = std::string(field.value());
        }
        return response;

    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return {500, e.what(), {}};
    }
}

} // namespace http1
