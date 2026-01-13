#pragma once

#include "Http2Server.h"
#include "Http2ServerError.h"

#include <Result.h>
#include <atomic>
#include <nghttp2/asio_http2_server.h>
#include <string>

namespace astra::http2 {

class NgHttp2Server {
public:
  NgHttp2Server(const ::http2::ServerConfig &config);
  ~NgHttp2Server();

  void handle(const std::string &method, const std::string &path,
              Http2Server::Handler handler);

  astra::outcome::Result<void, Http2ServerError> start();
  astra::outcome::Result<void, Http2ServerError> join();
  astra::outcome::Result<void, Http2ServerError> stop();

private:
  ::http2::ServerConfig m_config;
  std::atomic<bool> m_is_running{false};
  nghttp2::asio_http2::server::http2 m_server;
};

} // namespace astra::http2
