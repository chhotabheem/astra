#include "Http2Client.h"

#include "ClientRegistry.h"

namespace astra::http2 {

class Http2Client::Impl {
public:
  explicit Impl(const ::http2::ClientConfig &config) : m_registry(config) {
  }

  void submit(const std::string &host, uint16_t port, const std::string &method,
              const std::string &path, const std::string &body,
              const std::map<std::string, std::string> &headers,
              ResponseHandler handler) {
    auto client = m_registry.get_or_create(host, port);
    client->submit(method, path, body, headers, handler);
  }

private:
  ClientRegistry m_registry;
};

Http2Client::Http2Client(const ::http2::ClientConfig &config)
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
