#pragma once

#include <any>
#include <cstdint>
#include <obs/Context.h>

namespace astra::execution {

enum class JobType {
    TASK,
    SHUTDOWN
};

struct Job {
    JobType type;
    uint64_t session_id;
    std::any payload;
    obs::Context trace_ctx;
    
    static Job shutdown() {
        return Job{JobType::SHUTDOWN, 0, {}, obs::Context{}};
    }
};

} // namespace astra::execution
