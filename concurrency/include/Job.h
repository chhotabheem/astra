#pragma once

#include <any>
#include <cstdint>
#include <obs/Context.h>

namespace astra::concurrency {

/**
 * @brief Represents the type of job/event in the system.
 */
enum class JobType {
    HTTP_REQUEST,
    DB_RESPONSE,
    CLIENT_RESPONSE,
    FSM_EVENT,
    SHUTDOWN
};

/**
 * @brief The unified unit of work for the Worker Pool.
 * 
 * Uses std::any for type-safe payload storage.
 * trace_ctx carries observability context across thread boundaries.
 */
struct Job {
    JobType type;
    uint64_t session_id;     // For Sharding/Affinity
    std::any payload;        // Type-safe container for ANY object
    obs::Context trace_ctx;  // Observability context for distributed tracing
    
    /// Create a shutdown signal job (internal use only)
    static Job shutdown() {
        return Job{JobType::SHUTDOWN, 0, {}, obs::Context{}};
    }
};

} // namespace astra::concurrency

