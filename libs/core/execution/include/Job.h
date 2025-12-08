#pragma once

#include <any>
#include <cstdint>
#include <obs/Context.h>

namespace astra::execution {

enum class JobType {
    TASK
};

struct Job {
    JobType type;
    uint64_t session_id;
    std::any payload;
    obs::Context trace_ctx;

};

} // namespace astra::execution
