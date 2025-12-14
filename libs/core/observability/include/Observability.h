#pragma once

// Main observability API - Provider pattern
#include "Provider.h"
#include "Tracer.h"
#include "Context.h"
#include "Span.h"
#include "Log.h"
#include "Metrics.h"
#include "MetricsRegistry.h"

namespace obs {

// Convenience initialization functions
// Usage (C++17):
//   ::observability::Config config;
//   config.set_service_name("my_service");
//   config.set_otlp_endpoint("http://localhost:4317");
//   obs::init(config);
//   
//   auto tracer = obs::Provider::instance().get_tracer("my_service");
//   auto span = tracer->start_span("operation");
//   span->attr("key", "value");
//   span->end();
//   
//   obs::info("Log message");
//   

} // namespace obs

