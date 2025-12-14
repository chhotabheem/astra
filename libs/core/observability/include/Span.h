#pragma once

#include <Context.h>
#include <string_view>
#include <memory>
#include <chrono>
#include <initializer_list>
#include <utility>

namespace obs {

// Forward declarations
class Provider;
class TracerImpl;

// Span status codes (OpenTelemetry standard)
enum class StatusCode {
    Unset,   // Default - no explicit status set
    Ok,      // Operation completed successfully
    Error    // Operation failed
};

// Span kind (OpenTelemetry standard)
enum class SpanKind {
    Internal,   // Default - internal operation
    Server,     // Server-side request handler
    Client,     // Client-side request
    Producer,   // Message producer
    Consumer    // Message consumer
};

// Attributes for spans (string_view for zero-copy at call site)
using Attributes = std::initializer_list<std::pair<std::string_view, std::string_view>>;

/**
 * Span - Represents a unit of work in a trace
 * 
 * Spans are created via Tracer::start_span() and returned as shared_ptr.
 * Call end() when the work is complete:
 * 
 *   auto tracer = obs::Provider::instance().get_tracer("my-service");
 *   auto span = tracer->start_span("operation");
 *   // ... async work ...
 *   span->end();
 */
class Span {
public:
    // Destructor - auto-ends if not already ended (with warning)
    ~Span();
    
    // Move-only (no copy)
    Span(Span&&) noexcept;
    Span& operator=(Span&&) noexcept;
    Span(const Span&) = delete;
    Span& operator=(const Span&) = delete;
    
    // Fluent API - set attributes (returns *this for chaining)
    Span& attr(std::string_view key, std::string_view value);
    Span& attr(std::string_view key, int64_t value);
    Span& attr(std::string_view key, double value);
    Span& attr(std::string_view key, bool value);
    
    // Set span status
    Span& set_status(StatusCode code, std::string_view message = "");
    
    // Set span kind (Server, Client, Internal, Producer, Consumer)
    Span& kind(SpanKind kind);
    
    // Add timestamped event with optional attributes
    Span& add_event(std::string_view name);
    Span& add_event(std::string_view name, Attributes attrs);
    
    // Explicit end - call when async work completes
    // Safe to call multiple times (no-op after first call)
    void end();
    
    // Check if span has been ended
    bool is_ended() const;
    
    // Get span context (for propagation)
    Context context() const;
    
    // Check if span is recording (sampling)
    bool is_recording() const;
    
    // Pimpl - hide OTel SDK details
    struct Impl;
    
private:
    friend class TracerImpl;
    friend class Provider;
    
    // Private constructor (only callable by TracerImpl)
    explicit Span(Impl* impl);
    
    std::unique_ptr<Impl> m_impl;
    bool m_ended = false;
};

} // namespace obs
