#include "Http2Response.h"

#include "Http2ResponseWriter.h"

#include <Log.h>

namespace astra::http2 {

Http2Response::Http2Response(std::weak_ptr<Http2ResponseWriter> handle)
    : m_writer(std::move(handle)) {
}

void Http2Response::set_status(int code) noexcept {
  m_status = code;
}

void Http2Response::set_header(const std::string &key,
                               const std::string &value) {
  m_headers[key] = value;
}

void Http2Response::write(const std::string &data) {
  m_body.append(data);
}

void Http2Response::close() {
  if (m_closed) {
    return;
  }
  m_closed = true;

  if (auto handle = m_writer.lock()) {
    int status = m_status.value_or(500);
    if (!m_status.has_value()) {
      obs::warn("Http2Response closed without setting status code - defaulting "
                "to 500");
    }
    handle->send(status, std::move(m_headers), std::move(m_body));
  } else {
    obs::debug("Cannot send response: stream already closed");
  }
}

void Http2Response::add_scoped_resource(
    std::unique_ptr<astra::execution::IScopedResource> resource) {
  if (auto handle = m_writer.lock()) {
    handle->add_scoped_resource(std::move(resource));
  }
}

bool Http2Response::is_alive() const noexcept {
  if (auto handle = m_writer.lock()) {
    return handle->is_alive();
  }
  return false;
}

} // namespace astra::http2
