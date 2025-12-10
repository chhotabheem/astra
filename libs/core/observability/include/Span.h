#pragma once

#include <Context.h>
#include <string_view>
#include <memory>
#include <chrono>
#include <initializer_list>
#include <utility>

namespace obs {

// Forward declaration
class Provider;

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
 * RAII Span - Move-only value type
 * 
 * Usage:
 *   {
 *       auto span = obs::span("operation");
 *       span.attr("key", "value");
 *       span.kind(SpanKind::Server);
 *       
 *       // ... do work ...
 *       
 *       span.set_status(StatusCode::Ok);
 *   } // Span auto-ends here (RAII)
 */
class Span {
public:
    // Destructor - auto-ends span
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
    
    // Get span context (for propagation)
    Context context() const;
    
    // Check if span is recording (sampling)
    bool is_recording() const;
    
    // Pimpl - hide OTel SDK details
    struct Impl;
    
private:
    friend Span span(std::string_view name);
    friend Span span(std::string_view name, const Context& parent);
    friend class Provider;
    
    // Private constructor (only callable by span() functions)
    explicit Span(Impl* impl);
    
    std::unique_ptr<Impl> m_impl;
};

// Span creation functions
Span span(std::string_view name);
Span span(std::string_view name, const Context& parent);

} // namespace obs
