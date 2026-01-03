#include "Http2ResponseWriter.h"

namespace astra::http2 {

Http2ResponseWriter::Http2ResponseWriter(SendResponse send_response,
                                         PostWork post_work)
    : m_send_response(std::move(send_response)),
      m_post_work(std::move(post_work)), m_stream_alive(true) {
}

void Http2ResponseWriter::send(int status,
                               std::map<std::string, std::string> headers,
                               std::string body) {
  auto self = shared_from_this();

  m_post_work([self, status, headers = std::move(headers),
               body = std::move(body)]() mutable {
    if (self->m_stream_alive.load(std::memory_order_acquire)) {
      self->m_send_response(status, std::move(headers), std::move(body));
    }
  });
}

void Http2ResponseWriter::mark_closed() noexcept {
  m_stream_alive.store(false, std::memory_order_release);
}

bool Http2ResponseWriter::is_alive() const noexcept {
  return m_stream_alive.load(std::memory_order_acquire);
}

} // namespace astra::http2
