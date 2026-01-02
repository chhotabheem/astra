#pragma once

// Main observability API - Provider pattern
#include "Context.h"
#include "Log.h"
#include "Metrics.h"
#include "MetricsRegistry.h"
#include "Provider.h"
#include "Span.h"
#include "Tracer.h"

namespace astra::observability {

// Convenience initialization functions
// Usage (C++17):
//   ::observability::Config config;
//   config.set_service_name("my_service");
//   config.set_otlp_endpoint("http://localhost:4317");
//   astra::observability::init(config);
//
//   auto tracer =
//   astra::observability::Provider::instance().get_tracer("my_service"); auto
//   span = tracer->start_span("operation"); span->attr("key", "value");
//   span->end();
//
//   astra::observability::info("Log message");
//

} // namespace astra::observability

// Backward compatibility alias - allows existing code using obs:: to continue
// working
namespace obs = astra::observability;
