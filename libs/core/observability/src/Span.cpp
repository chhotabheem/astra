#include "ProviderImpl.h"
#include "SpanImpl.h"

#include <Log.h>
#include <Provider.h>
#include <Span.h>
#include <cstring>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/tracer.h>

namespace astra::observability {

// Span::Impl methods
void Span::Impl::end_span() {
  if (!ended && otel_span) {
    otel_span->End();

    // Pop from active span stack
    auto &provider = Provider::instance();
    provider.impl().pop_active_span();
    ended = true;
  }
}

Span::Impl::~Impl() {
  // Auto-end if not explicitly ended (RAII fallback)
  if (!ended && otel_span) {
    astra::observability::warn(
        "Span destroyed without explicit end() - auto-ending");
    end_span();
  }
}

// Constructor
Span::Span(Impl *impl) : m_impl(impl) {
}

// Destructor
Span::~Span() = default;

// Move constructor
Span::Span(Span &&other) noexcept
    : m_impl(std::move(other.m_impl)), m_ended(other.m_ended) {
  other.m_ended = true; // Prevent double-end
}

// Move assignment
Span &Span::operator=(Span &&other) noexcept {
  if (this != &other) {
    m_impl = std::move(other.m_impl);
    m_ended = other.m_ended;
    other.m_ended = true;
  }
  return *this;
}

// Explicit end
void Span::end() {
  if (!m_ended && m_impl) {
    m_impl->end_span();
    m_ended = true;
  }
}

// Check if ended
bool Span::is_ended() const {
  return m_ended || (m_impl && m_impl->ended);
}

// Attributes
Span &Span::attr(const std::string &key, const std::string &value) {
  if (m_impl && m_impl->otel_span && !m_ended) {
    m_impl->otel_span->SetAttribute(key, value);
  }
  return *this;
}

Span &Span::attr(const std::string &key, int64_t value) {
  if (m_impl && m_impl->otel_span && !m_ended) {
    m_impl->otel_span->SetAttribute(key, value);
  }
  return *this;
}

Span &Span::attr(const std::string &key, double value) {
  if (m_impl && m_impl->otel_span && !m_ended) {
    m_impl->otel_span->SetAttribute(key, value);
  }
  return *this;
}

Span &Span::attr(const std::string &key, bool value) {
  if (m_impl && m_impl->otel_span && !m_ended) {
    m_impl->otel_span->SetAttribute(key, value);
  }
  return *this;
}

// Status
Span &Span::set_status(StatusCode code, const std::string &message) {
  if (m_impl && m_impl->otel_span && !m_ended) {
    trace_api::StatusCode otel_code;
    switch (code) {
    case StatusCode::Unset:
      otel_code = trace_api::StatusCode::kUnset;
      break;
    case StatusCode::Ok:
      otel_code = trace_api::StatusCode::kOk;
      break;
    case StatusCode::Error:
      otel_code = trace_api::StatusCode::kError;
      break;
    default:
      otel_code = trace_api::StatusCode::kUnset;
    }
    m_impl->otel_span->SetStatus(otel_code, message);
  }
  return *this;
}

// Kind
Span &Span::kind(SpanKind kind) {
  if (m_impl && m_impl->otel_span && !m_ended) {
    const char *kind_str = "internal";
    switch (kind) {
    case SpanKind::Internal:
      kind_str = "internal";
      break;
    case SpanKind::Server:
      kind_str = "server";
      break;
    case SpanKind::Client:
      kind_str = "client";
      break;
    case SpanKind::Producer:
      kind_str = "producer";
      break;
    case SpanKind::Consumer:
      kind_str = "consumer";
      break;
    }
    m_impl->otel_span->SetAttribute("span.kind", kind_str);
  }
  return *this;
}

// Events
Span &Span::add_event(const std::string &name) {
  if (m_impl && m_impl->otel_span && !m_ended) {
    m_impl->otel_span->AddEvent(name);
  }
  return *this;
}

Span &Span::add_event(const std::string &name, Attributes attrs) {
  if (m_impl && m_impl->otel_span && !m_ended) {
    // Build OTel attributes map
    std::map<std::string, opentelemetry::common::AttributeValue> otel_attrs;
    for (const auto &attr : attrs) {
      otel_attrs[attr.first] = attr.second;
    }
    m_impl->otel_span->AddEvent(name, otel_attrs);
  }
  return *this;
}

// Context extraction
Context Span::context() const {
  if (!m_impl) {
    return Context{};
  }
  return m_impl->span_context;
}

// Recording check
bool Span::is_recording() const {
  if (m_impl && m_impl->otel_span && !m_ended) {
    return m_impl->otel_span->IsRecording();
  }
  return false;
}

} // namespace astra::observability
