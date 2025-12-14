#include "Http2ClientPool.h"
#include <algorithm>

namespace astra::http2 {

Http2ClientPool::Http2ClientPool(const ClientConfig& config) {
    // Use at least 1 client
    size_t pool_size = std::max(static_cast<size_t>(1), 
                                 static_cast<size_t>(config.pool_size()));
    
    m_clients.reserve(pool_size);
    
    for (size_t i = 0; i < pool_size; ++i) {
        m_clients.push_back(std::make_unique<Client>(config));
    }
}

Client& Http2ClientPool::get() {
    // Atomic increment with wrap-around (round-robin)
    size_t index = m_next.fetch_add(1, std::memory_order_relaxed) % m_clients.size();
    return *m_clients[index];
}

size_t Http2ClientPool::size() const noexcept {
    return m_clients.size();
}

} // namespace astra::http2
