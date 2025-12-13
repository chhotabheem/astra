#pragma once

#include "StickyQueue.h"
#include "IMessageHandler.h"
#include <MetricsRegistry.h>
#include <chrono>

namespace astra::execution {

/**
 * @brief Observable decorator for StickyQueue.
 * 
 * Wraps a StickyQueue and adds observability:
 * - Counter: message_pool.submitted
 * - Counter: message_pool.delivered
 * - Gauge: message_pool.queue_depth
 * - Histogram: message_pool.latency_ms
 */
class ObservableMessagePool {
public:
    /**
     * @brief Construct observable pool wrapper.
     * @param pool The underlying pool to wrap
     */
    explicit ObservableMessagePool(StickyQueue& pool);
    
    void start();
    void stop();
    bool submit(Message msg);
    
    [[nodiscard]] size_t worker_count() const { return m_pool.worker_count(); }

private:
    StickyQueue& m_pool;
    obs::MetricsRegistry m_metrics;
};

/**
 * @brief Observable handler wrapper that tracks delivery metrics.
 * 
 * Wraps an IMessageHandler and tracks:
 * - Delivery count
 * - Queue depth (decrement)
 * - Latency histogram
 */
class ObservableHandlerWrapper : public IMessageHandler {
public:
    ObservableHandlerWrapper(IMessageHandler& inner, obs::Gauge queue_depth);
    
    void handle(Message& msg) override;

private:
    IMessageHandler& m_inner;
    obs::Gauge m_queue_depth;  // Shared with pool (lightweight wrapper)
    obs::MetricsRegistry m_metrics;
};

} // namespace astra::execution
