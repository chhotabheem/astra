#pragma once

#include <any>
#include <cstdint>
#include <Context.h>

namespace astra::execution {

/**
 * @brief Message to be delivered by StripedMessagePool.
 * 
 * Unlike Job (which is executed), Message is delivered to IMessageHandler.
 * The handler decides what to do with the payload.
 * 
 * Fields:
 * - session_id: Used for session affinity (same session → same worker)
 * - trace_ctx: Observability context propagated across thread boundaries
 * - payload: Type-erased payload (application casts to its variant type)
 */
struct Message {
    uint64_t session_id;
    obs::Context trace_ctx;
    std::any payload;
};

} // namespace astra::execution
