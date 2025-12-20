#include "Http2Client.h"
#include "ClientDispatcher.h"

namespace astra::http2 {

Http2Client::Http2Client(const ClientConfig& config)
    : m_dispatcher(std::make_unique<ClientDispatcher>(config)) {}

Http2Client::~Http2Client() = default;

void Http2Client::submit(const std::string& host, uint16_t port,
                         const std::string& method, const std::string& path,
                         const std::string& body,
                         const std::map<std::string, std::string>& headers,
                         ResponseHandler handler) {
    m_dispatcher->submit(host, port, method, path, body, headers, handler);
}

}
