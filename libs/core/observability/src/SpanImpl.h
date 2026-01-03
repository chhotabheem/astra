#pragma once

#include <Span.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/span.h>

namespace astra::observability {

// Span implementation details - visible to TracerImpl
struct Span::Impl {
  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> otel_span;
  Context span_context;
  bool ended = false;

  Impl(opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span,
       const Context &ctx)
      : otel_span(std::move(span)), span_context(ctx) {
  }

  void end_span();
  ~Impl();
};

} // namespace astra::observability
