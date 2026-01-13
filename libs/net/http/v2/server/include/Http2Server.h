#pragma once

#include "Http2ServerError.h"
#include "IHttp2Server.h"
#include "IRouter.h"
#include "http2server.pb.h"

#include <Result.h>
#include <functional>
#include <memory>
#include <string>

namespace astra::http2 {

class Http2Request;
class Http2Response;

class Http2Server : public IHttp2Server {
public:
  Http2Server(const ::http2::ServerConfig &config,
              astra::router::IRouter &router);
  ~Http2Server() override;

  Http2Server(const Http2Server &) = delete;
  Http2Server &operator=(const Http2Server &) = delete;

  void handle(const std::string &method, const std::string &path,
              Handler handler) override;

  astra::outcome::Result<void, Http2ServerError> start() override;
  astra::outcome::Result<void, Http2ServerError> join() override;
  astra::outcome::Result<void, Http2ServerError> stop() override;

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
  astra::router::IRouter &m_router;
};

} // namespace astra::http2
