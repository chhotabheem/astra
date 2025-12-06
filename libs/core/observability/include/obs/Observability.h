#pragma once
// =============================================================================
// obs/Observability.h - Main include file for observability library
// =============================================================================
//
// This is the only header business logic needs to include.
// All OTel details are hidden in the implementation.
//
// Usage:
//   #include <obs/Observability.h>
//
//   void handle_request(Job& job) {
//       auto span = obs::span("handle_request", job.trace_ctx);
//       span.attr("session_id", job.session_id);
//
//       obs::info("Processing", job.trace_ctx);
//       obs::counter("requests").inc();
//   }
//
// =============================================================================

// Core context (flows with Jobs in SEDA)
#include "Context.h"

// Tracing
#include "Span.h"

// Logging
#include "Log.h"

// Metrics
#include "Metrics.h"

// Backend interface (for set_backend() DI)
#include "IBackend.h"

// Console backend (for debugging/tests)
#include "ConsoleBackend.h"

namespace obs {

// =============================================================================
// Configuration (Day 0 - set once at startup)
// =============================================================================
struct Config {
    std::string service_name;
    std::string service_version;
    std::string environment;       // dev, staging, prod
    double sampling_rate{1.0};     // 0.0 to 1.0 (1.0 = 100% sampled)
    
    // Exporter endpoints (empty = use OStream/console)
    std::string otlp_endpoint;     // e.g., "http://collector:4317"
};

/// Initialize observability (call once at startup)
void init(const Config& config);

/// Shutdown observability (call before exit)
void shutdown();

/// Check if initialized
bool is_initialized();

} // namespace obs
