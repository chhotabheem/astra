#pragma once
#include <string>

namespace obs {

struct Config {
    std::string service_name;
    std::string service_version{"1.0.0"};
    std::string environment{"production"};
    std::string otlp_endpoint{"http://localhost:4317"};
    bool enable_metrics{true};
    bool enable_tracing{true};
    bool enable_logging{true};
};

} // namespace obs
