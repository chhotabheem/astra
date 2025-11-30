#include <any>
#include <cstdint>

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
 */
struct Job {
    JobType type;
    uint64_t session_id; // For Sharding/Affinity
    std::any payload;    // Type-safe container for ANY object
};

} // namespace astra::concurrency
