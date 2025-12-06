#pragma once
// =============================================================================
// obs/IBackend.h - Backend interface for Dependency Injection
// =============================================================================
//
// This enables:
// - Production: OTelBackend (OpenTelemetry)
// - Tests: MockBackend or NoopBackend
// - Future: DatadogBackend, JaegerBackend, etc.
//
// =============================================================================

#include "Context.h"
#include <string_view>
#include <memory>

namespace obs {

// Forward declarations
class Span;
class Counter;
class Histogram;

/// Log levels
enum class Level {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

/// Backend interface - implement this for different telemetry providers.
/// Set at process boot via obs::set_backend().
class IBackend {
public:
    virtual ~IBackend() = default;
    
    // Lifecycle
    virtual void shutdown() = 0;
    
    // Tracing
    virtual std::unique_ptr<Span> create_span(std::string_view name, const Context& ctx) = 0;
    virtual std::unique_ptr<Span> create_root_span(std::string_view name) = 0;
    
    // Logging
    virtual void log(Level level, std::string_view message, const Context& ctx) = 0;
    
    // Metrics
    virtual std::shared_ptr<Counter> get_counter(std::string_view name, std::string_view desc) = 0;
    virtual std::shared_ptr<Histogram> get_histogram(std::string_view name, std::string_view desc) = 0;
};

/// Set the backend (call once at process startup)
void set_backend(std::unique_ptr<IBackend> backend);

/// Shutdown observability (flushes and stops backend)
void shutdown();

} // namespace obs
