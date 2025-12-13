#pragma once

#include <any>
#include <cstdint>
#include <Context.h>

namespace astra::execution {

/**
 * @brief Message for queue-based delivery.
 * 
 * Used by both SharedQueue and StickyQueue:
 * - SharedQueue: workers consume from shared queue
 * - StickyQueue: same session_id routes to same worker
 * 
 * Fields:
 * - session_id: Used for routing (StickyQueue uses for affinity)
 * - trace_ctx: Observability context propagated across thread boundaries
 * - payload: Type-erased payload (can be data or std::function<void()>)
 */
struct Message {
    uint64_t session_id;
    obs::Context trace_ctx;
    std::any payload;
};

} // namespace astra::execution
