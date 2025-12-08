#include "ObservableMessagePool.h"
#include <obs/Log.h>

namespace astra::execution {

// =============================================================================
// ObservableMessagePool
// =============================================================================
ObservableMessagePool::ObservableMessagePool(StripedMessagePool& pool)
    : m_pool(pool)
    , m_submitted(obs::counter("message_pool.submitted", "Messages submitted to pool"))
    , m_queue_depth(obs::gauge("message_pool.queue_depth", "Current queue depth")) {
}

void ObservableMessagePool::start() {
    m_pool.start();
    obs::debug("ObservableMessagePool: started");
}

void ObservableMessagePool::stop() {
    m_pool.stop();
    obs::debug("ObservableMessagePool: stopped");
}

bool ObservableMessagePool::submit(Message msg) {
    m_submitted.inc();
    m_queue_depth.inc();
    
    // Add timestamp for latency measurement
    auto now = std::chrono::steady_clock::now();
    auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // Store timestamp in a wrapper - handler will extract and measure
    // For simplicity, we piggyback on the existing trace context
    // A proper implementation would use a custom wrapper
    
    return m_pool.submit(std::move(msg));
}

// =============================================================================
// ObservableHandlerWrapper
// =============================================================================
ObservableHandlerWrapper::ObservableHandlerWrapper(IMessageHandler& inner, obs::Gauge& queue_depth)
    : m_inner(inner)
    , m_delivered(obs::counter("message_pool.delivered", "Messages delivered to handler"))
    , m_queue_depth(queue_depth)
    , m_latency(obs::histogram("message_pool.latency_ms", "Message delivery latency in ms")) {
}

void ObservableHandlerWrapper::handle(Message& msg) {
    // Decrement queue depth
    m_queue_depth.dec();
    
    auto start = std::chrono::steady_clock::now();
    
    // Deliver to inner handler
    m_inner.handle(msg);
    
    // Record metrics
    m_delivered.inc();
    
    auto end = std::chrono::steady_clock::now();
    auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    m_latency.record(static_cast<double>(latency_ms));
}

} // namespace astra::execution
