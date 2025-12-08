#pragma once

#include "StripedMessagePool.h"
#include "IMessageHandler.h"
#include <obs/Metrics.h>
#include <chrono>

namespace astra::execution {

/**
 * @brief Observable decorator for StripedMessagePool.
 * 
 * Wraps a StripedMessagePool and adds observability:
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
    explicit ObservableMessagePool(StripedMessagePool& pool);
    
    void start();
    void stop();
    bool submit(Message msg);
    
    [[nodiscard]] size_t thread_count() const { return m_pool.thread_count(); }

private:
    StripedMessagePool& m_pool;
    
    obs::Counter& m_submitted;
    obs::Gauge& m_queue_depth;
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
    ObservableHandlerWrapper(IMessageHandler& inner, obs::Gauge& queue_depth);
    
    void handle(Message& msg) override;

private:
    IMessageHandler& m_inner;
    obs::Counter& m_delivered;
    obs::Gauge& m_queue_depth;
    obs::Histogram& m_latency;
};

} // namespace astra::execution
