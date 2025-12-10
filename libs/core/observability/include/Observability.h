#pragma once

// Main observability API - Provider pattern
#include "Provider.h"
#include "Config.h"
#include "Context.h"
#include "Span.h"
#include "Log.h"
#include "Metrics.h"
#include "MetricsRegistry.h"

namespace obs {

// Convenience initialization functions
// Usage:
//   obs::Config config{
//       .service_name = "my_service",
//       .otlp_endpoint = "http://localhost:4317"
//   };
//   obs::init(config);
//   
//   auto span = obs::span("operation");
//   obs::info("Log message");
//   

} // namespace obs
