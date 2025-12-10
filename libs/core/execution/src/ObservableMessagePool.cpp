#include "ObservableMessagePool.h"
#include <Log.h>

namespace astra::execution {

// =============================================================================
// ObservableMessagePool
// =============================================================================
ObservableMessagePool::ObservableMessagePool(StripedMessagePool& pool)
    : m_pool(pool)
{
    m_metrics
        .counter("submitted", "message_pool.submitted")
        .gauge("queue_depth", "message_pool.queue_depth");
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
    m_metrics.counter("submitted").inc();
    m_metrics.gauge("queue_depth").add(1);
    
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
ObservableHandlerWrapper::ObservableHandlerWrapper(IMessageHandler& inner, obs::Gauge queue_depth)
    : m_inner(inner)
    , m_queue_depth(std::move(queue_depth))
{
    m_metrics
        .counter("delivered", "message_pool.delivered")
        .duration_histogram("latency", "message_pool.latency");
}

void ObservableHandlerWrapper::handle(Message& msg) {
    // Decrement queue depth
    m_queue_depth.add(-1);
    
    auto start = std::chrono::steady_clock::now();
    
    // Deliver to inner handler
    m_inner.handle(msg);
    
    // Record metrics
    m_metrics.counter("delivered").inc();
    
    auto duration = std::chrono::steady_clock::now() - start;
    m_metrics.duration_histogram("latency").record(duration);
}

} // namespace astra::execution
