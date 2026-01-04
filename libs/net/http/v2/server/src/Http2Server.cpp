#include "Http2Server.h"

#include "Http2Request.h"
#include "Http2Response.h"
#include "NgHttp2Server.h"

namespace astra::http2 {

class Http2Server::Impl {
public:
  Impl(const ServerConfig &config) : backend(config) {
  }

  NgHttp2Server backend;
};

Http2Server::Http2Server(const ServerConfig &config)
    : m_impl(std::make_unique<Impl>(config)) {
  m_impl->backend.handle("*", "/",
                         [this](std::shared_ptr<astra::router::IRequest> req,
                                std::shared_ptr<astra::router::IResponse> res) {
                           m_router.dispatch(req, res);
                         });
}

Http2Server::~Http2Server() = default;

void Http2Server::handle(const std::string &method, const std::string &path,
                         Handler handler) {
  m_impl->backend.handle(method, path, handler);
}

astra::outcome::Result<void, Http2ServerError> Http2Server::start() {
  return m_impl->backend.start();
}

astra::outcome::Result<void, Http2ServerError> Http2Server::join() {
  return m_impl->backend.join();
}

astra::outcome::Result<void, Http2ServerError> Http2Server::stop() {
  return m_impl->backend.stop();
}

} // namespace astra::http2
