#pragma once

#include <boost/beast/http.hpp>
#include <map>
#include <string>

namespace astra::http1 {

struct Response {
  int status_code;
  std::string body;
  std::map<std::string, std::string> headers;
};

class Client {
public:
  Client();
  ~Client();

  [[nodiscard]] Response get(const std::string &host, const std::string &port,
                             const std::string &target);
  [[nodiscard]] Response post(const std::string &host, const std::string &port,
                              const std::string &target,
                              const std::string &body);

private:
  Response perform_request(const std::string &host, const std::string &port,
                           const std::string &target,
                           boost::beast::http::verb method,
                           const std::string &body);
};

} // namespace astra::http1
