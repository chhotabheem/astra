#pragma once
#include <string>
#include <string_view>

namespace obs {

/// Initialization parameters for the observability library.
/// Apps populate this struct and pass to obs::init().
struct InitParams {
    std::string service_name;
    std::string service_version{"1.0.0"};
    std::string environment{"production"};
    std::string otlp_endpoint{"http://localhost:4317"};
    bool enable_metrics{true};
    bool enable_tracing{true};
    bool enable_logging{true};
};

// Backward compatibility alias - deprecated, use InitParams instead
using Config = InitParams;

} // namespace obs
