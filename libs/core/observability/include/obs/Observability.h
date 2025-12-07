#pragma once

#include "Context.h"
#include "Span.h"
#include "Log.h"
#include "Metrics.h"
#include "IBackend.h"
#include "ConsoleBackend.h"

namespace obs {

struct Config {
    std::string service_name;
    std::string service_version;
    std::string environment;        // dev, staging, prod
    double sampling_rate{1.0};      // 0.0 to 1.0 (1.0 = 100% sampled)
    std::string otlp_endpoint;      // e.g., "http://collector:4317"
};

void init(const Config& config);
void shutdown();
bool is_initialized();

} // namespace obs
