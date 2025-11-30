#include "ResponseHandle.h"
#include <boost/asio/post.hpp>

namespace http2server {

ResponseHandle::ResponseHandle(SendFunction send_fn, boost::asio::io_context& io_ctx)
    : m_send_fn(std::move(send_fn))
    , m_io_ctx(io_ctx)
    , m_stream_alive(true) {
}

void ResponseHandle::send(std::string data) {
    // Post to io_context where response object lives
    // Capture shared_ptr to keep handle alive until lambda executes
    auto self = shared_from_this();
    
    boost::asio::post(m_io_ctx, [self, data = std::move(data)]() {
        // On io_context thread now - safe to access response
        if (self->m_stream_alive.load(std::memory_order_acquire)) {
            self->m_send_fn(std::move(data));
        }
        // else: stream closed, drop response gracefully
    });
}

void ResponseHandle::mark_closed() noexcept {
    m_stream_alive.store(false, std::memory_order_release);
}

bool ResponseHandle::is_alive() const noexcept {
    return m_stream_alive.load(std::memory_order_acquire);
}

} // namespace http2server
