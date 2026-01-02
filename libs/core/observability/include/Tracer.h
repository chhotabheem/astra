#pragma once

#include <Context.h>
#include <memory>
#include <string>

namespace astra::observability {

class Span;

/**
 * Tracer - Factory for creating spans
 *
 * Usage:
 *   auto tracer = obs::Provider::instance().get_tracer("my-service");
 *   auto span = tracer->start_span("operation");
 *   // ... async work ...
 *   span->end();
 *
 * Tracers should be obtained once and injected via DI.
 */
class Tracer {
public:
  virtual ~Tracer() = default;

  // Create a new span (uses current active context as parent)
  [[nodiscard]] virtual std::shared_ptr<Span>
  start_span(const std::string &name) = 0;

  // Create a new span with explicit parent context
  [[nodiscard]] virtual std::shared_ptr<Span>
  start_span(const std::string &name, const Context &parent) = 0;

  // Get tracer name/service
  [[nodiscard]] virtual const std::string &name() const = 0;
};

} // namespace astra::observability
