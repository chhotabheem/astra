#pragma once

#include <Context.h>
#include <chrono>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

namespace astra::observability {

// Forward declarations
class Provider;
class TracerImpl;

// Span status codes (OpenTelemetry standard)
enum class StatusCode {
  Unset, // Default - no explicit status set
  Ok,    // Operation completed successfully
  Error  // Operation failed
};

// Span kind (OpenTelemetry standard)
enum class SpanKind {
  Internal, // Default - internal operation
  Server,   // Server-side request handler
  Client,   // Client-side request
  Producer, // Message producer
  Consumer  // Message consumer
};

// Attributes for spans
using Attributes = std::initializer_list<std::pair<std::string, std::string>>;

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
  Span(Span &&) noexcept;
  Span &operator=(Span &&) noexcept;
  Span(const Span &) = delete;
  Span &operator=(const Span &) = delete;

  // Fluent API - set attributes (returns *this for chaining)
  Span &attr(const std::string &key, const std::string &value);
  Span &attr(const std::string &key, int64_t value);
  Span &attr(const std::string &key, double value);
  Span &attr(const std::string &key, bool value);

  // Set span status
  Span &set_status(StatusCode code, const std::string &message = "");

  // Set span kind (Server, Client, Internal, Producer, Consumer)
  Span &kind(SpanKind kind);

  // Add timestamped event with optional attributes
  Span &add_event(const std::string &name);
  Span &add_event(const std::string &name, Attributes attrs);

  // Explicit end - call when async work completes
  // Safe to call multiple times (no-op after first call)
  void end();

  // Check if span has been ended
  [[nodiscard]] bool is_ended() const;

  // Get span context (for propagation)
  [[nodiscard]] Context context() const;

  // Check if span is recording (sampling)
  [[nodiscard]] bool is_recording() const;

  // Pimpl - hide OTel SDK details
  struct Impl;

private:
  friend class TracerImpl;
  friend class Provider;

  // Private constructor (only callable by TracerImpl)
  explicit Span(Impl *impl);

  std::unique_ptr<Impl> m_impl;
  bool m_ended = false;
};

} // namespace astra::observability
