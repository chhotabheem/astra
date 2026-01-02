#include "Http2Client.h"

#include "ClientDispatcher.h"

namespace astra::http2 {

class Http2Client::Impl {
public:
  explicit Impl(const ClientConfig &config) : m_dispatcher(config) {
  }

  void submit(const std::string &host, uint16_t port, const std::string &method,
              const std::string &path, const std::string &body,
              const std::map<std::string, std::string> &headers,
              ResponseHandler handler) {
    m_dispatcher.submit(host, port, method, path, body, headers, handler);
  }

private:
  ClientDispatcher m_dispatcher;
};

Http2Client::Http2Client(const ClientConfig &config)
    : m_impl(std::make_unique<Impl>(config)) {
}

Http2Client::~Http2Client() = default;

void Http2Client::submit(const std::string &host, uint16_t port,
                         const std::string &method, const std::string &path,
                         const std::string &body,
                         const std::map<std::string, std::string> &headers,
                         ResponseHandler handler) {
  m_impl->submit(host, port, method, path, body, headers, handler);
}

} // namespace astra::http2
