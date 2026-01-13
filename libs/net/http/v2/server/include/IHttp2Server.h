#pragma once

#include "Http2ServerError.h"
#include "IRouter.h"

#include <Result.h>
#include <functional>
#include <string>

namespace astra::http2 {

class IHttp2Server {
public:
  using Handler = astra::router::Handler;

  virtual ~IHttp2Server() = default;

  virtual void handle(const std::string &method, const std::string &path,
                      Handler handler) = 0;

  virtual astra::outcome::Result<void, Http2ServerError> start() = 0;
  virtual astra::outcome::Result<void, Http2ServerError> join() = 0;
  virtual astra::outcome::Result<void, Http2ServerError> stop() = 0;
};

} // namespace astra::http2
