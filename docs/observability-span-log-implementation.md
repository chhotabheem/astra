# Observability Span & Log Implementation Guide
## Complete Standalone Implementation Document

**Created:** 2025-12-10  
**Target:** C++17, OpenTelemetry SDK, High-TPS Cloud-Native Systems  
**Scope:** Implement missing Span and Log functionality in `obs::v2` namespace  
**Audience:** Any AI agent in any IDE (self-contained, no external dependencies needed)

---

## 📋 Table of Contents

1. [Current Status](#current-status)
2. [Implementation Overview](#implementation-overview)
3. [Phase 3: Span Implementation](#phase-3-span-implementation)
4. [Phase 4: Log Implementation](#phase-4-log-implementation)
5. [Phase 5: Cross-Cutting Concerns](#phase-5-cross-cutting-concerns)
6. [Critical Fixes](#critical-fixes)
7. [Testing](#testing)
8. [API Usage Examples](#api-usage-examples)

---

## Current Status

### ✅ Already Implemented (Phases 1-2)

**Location:** `/home/siddu/astra/libs/core/observability`

**Phase 1: Core Infrastructure (COMPLETE)**
- `include/obs/v2/Config.h` - Configuration structs
- `include/obs/v2/Provider.h` - Central provider singleton
- `src/v2/Provider.cpp` - Provider implementation
- `src/v2/ProviderImpl.h` - Pimpl pattern (hides OTel SDK)
- `src/v2/ProviderImpl.cpp` - OTel integration

**Phase 2: Metrics API (COMPLETE - 44 tests passing)**
- `include/obs/v2/Metrics.h` - Counter, Histogram, DurationHistogram, Gauge
  - Thread-local instruments (ZERO mutex on hot path)
  - Attributes support (OpenTelemetry labels/tags)
  - Type-safe units (Dimensionless, Milliseconds, Bytes, etc.)
  - Chrono duration support
- `include/obs/v2/MetricsRegistry.h` - Registry pattern for organized metrics
- `include/obs/v2/Context.h` - W3C TraceContext (trace_id, span_id, baggage)
- `src/v2/Context.cpp` - Context parsing/serialization

**Tests:**
- `test/v2/provider_test.cpp`
- `test/v2/metrics_test.cpp` (16 tests)
- `test/v2/metrics_registry_test.cpp` (8 tests)
- `test/v2/attributes_test.cpp` (6 tests)
- `test/v2/safety_test.cpp` (10 concurrent/integration tests)

### ❌ Missing (To Be Implemented)

**Phase 3:** Span (distributed tracing) - NOT in v2 namespace  
**Phase 4:** Log (structured logging) - NOT in v2 namespace  
**Phase 5:** Cross-cutting concerns (resource attributes, graceful shutdown)

> **Note:** Old implementations exist in legacy `obs` namespace ([Span.h](file:///home/siddu/astra/libs/core/observability/include/Span.h), [Log.h](file:///home/siddu/astra/libs/core/observability/include/Log.h)) but need to be **reimplemented** in v2 with new design:
> - RAII value types (not virtual interfaces)
> - Structured attributes
> - Automatic trace correlation
> - Thread-local storage
> - Zero mutex on hot path

---

## Implementation Overview

### Design Principles (From Expert Panel Review)

1. **Zero Mutex on Hot Path** - Thread-local OTel instruments
2. **RAII Value Types** - Spans auto-end in destructor (no manual cleanup)
3. **Fluent API** - `span.attr("key", "value")` (NOT pointer syntax `span->attr()`)
4. **Structured Logging** - Attributes, automatic trace correlation
5. **Encapsulation** - Provider owns all state, no global variables
6. **Type Safety** - Use C++17 features, strong typing
7. **Performance** - Target <15 cycles per operation (vs 510 with mutex)

### **CRITICAL: MetricsRegistry Pattern** 

One of the PRIMARY design goals was to avoid declaring hundreds of metric member variables.

**❌ BAD (Old Way):**
```cpp
class MyClass {
private:
    obs::Counter m_requests;
    obs::Counter m_errors;
    obs::Counter m_timeouts;
    obs::Counter m_retries;
    obs::Histogram m_latency;
    obs::Histogram m_payload_size;
    obs::Gauge m_active_connections;
    obs::Gauge m_queue_depth;
    // ... 92 more variables if you have 100 metrics! 😱
};
```

**✅ GOOD (MetricsRegistry):**
```cpp
class MyClass {
public:
    MyClass() {
        // Register ALL metrics in constructor - fluent API
        m_metrics
            .counter("requests", "http.requests.total")
            .counter("errors", "http.errors.total")
            .counter("timeouts", "http.timeouts.total")
            .counter("retries", "http.retries.total")
            .duration_histogram("latency", "http.latency")
            .histogram("payload_size", "http.payload_bytes", Unit::Bytes)
            .gauge("active_connections", "http.active_connections")
            .gauge("queue_depth", "http.queue_depth");
            // ... 92 more metrics, STILL just ONE member variable! ✅
    }
    
    void method() {
        // Fast lookup by short key
        m_metrics.counter("requests").inc();
        m_metrics.gauge("active_connections").add(1);
    }
    
private:
    obs::v2::MetricsRegistry m_metrics;  // ← ONE variable for ALL metrics!
};
```

**How It Works:**
- `MetricsRegistry` stores metrics in `std::unordered_map<string, Counter/Histogram/Gauge>`
- Registration in constructor: `m_metrics.counter("short_key", "full.metric.name")`
- Usage: `m_metrics.counter("short_key").inc()` - fast map lookup (~constant time)
- All metrics registered once, ultra-fast in hot path

### File Structure

```
libs/core/observability/
├── include/obs/v2/
│   ├── Config.h                     [EXISTS] Configuration structs
│   ├── Provider.h                   [EXISTS] Central provider
│   ├── Metrics.h                    [EXISTS] Counter, Histogram, Gauge
│   ├── MetricsRegistry.h            [EXISTS] Registry pattern
│   ├── Context.h                    [EXISTS - obs namespace] W3C TraceContext
│   ├── Span.h                       [CREATE] RAII span (value type)
│   └── Log.h                        [CREATE] Structured logging
│
├── src/v2/
│   ├── Provider.cpp                 [EXISTS]
│   ├── ProviderImpl.h               [EXISTS - MODIFY] Add tracer + logger
│   ├── ProviderImpl.cpp             [EXISTS - MODIFY] OTel setup
│   ├── Metrics.cpp                  [EXISTS]
│   ├── MetricsRegistry.cpp          [EXISTS]
│   ├── Context.cpp                  [EXISTS - obs namespace]
│   ├── Span.cpp                     [CREATE] Span implementation
│   └── Log.cpp                      [CREATE] Log implementation
│
└── test/v2/
    ├── provider_test.cpp            [EXISTS]
    ├── metrics_test.cpp             [EXISTS]
    ├── metrics_registry_test.cpp    [EXISTS]
    ├── attributes_test.cpp          [EXISTS]
    ├── safety_test.cpp              [EXISTS]
    ├── span_test.cpp                [CREATE] Span unit tests
    ├── log_test.cpp                 [CREATE] Log unit tests
    └── integration_test.cpp         [CREATE] Full signal correlation
```

---

## Phase 3: Span Implementation

### Overview

Implement distributed tracing with RAII value-type Span that:
- Auto-ends in destructor (no manual cleanup)
- Uses fluent API (NOT pointer syntax)
- Supports attributes, status, kind, events
- Integrates with OTel Tracer
- Manages active span context (thread-local stack)

### Step 3.1: Create Span Header

**File:** `include/obs/v2/Span.h`

```cpp
#pragma once
#include <Context.h>  // Reuse existing Context from obs namespace
#include <string_view>
#include <memory>
#include <chrono>
#include <initializer_list>
#include <utility>

namespace obs::v2 {

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

// Attributes for spans (reuse from Metrics.h)
using Attributes = std::initializer_list<std::pair<std::string_view, std::string_view>>;

/**
 * RAII Span - Move-only value type
 * 
 * Usage:
 *   {
 *       auto span = obs::v2::span("operation");
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
    obs::Context context() const;
    
    // Check if span is recording (sampling)
    bool is_recording() const;
    
private:
    friend Span span(std::string_view name);
    friend Span span(std::string_view name, const obs::Context& parent);
    friend class Provider;
    
    // Private constructor (only callable by span() functions)
    explicit Span(struct Impl* impl);
    
    // Pimpl - hide OTel SDK details
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Span creation functions
Span span(std::string_view name);
Span span(std::string_view name, const obs::Context& parent);

} // namespace obs::v2
```

### Step 3.2: Create Span Implementation

**File:** `src/v2/Span.cpp`

```cpp
#include <obs/v2/Span.h>
#include "ProviderImpl.h"
#include <obs/v2/Provider.h>

#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>

namespace obs::v2 {

namespace trace_api = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;

// Pimpl - hide OTel SDK types
struct Span::Impl {
    nostd::shared_ptr<trace_api::Span> otel_span;
    
    Impl(nostd::shared_ptr<trace_api::Span> span)
        : otel_span(std::move(span)) {}
    
    ~Impl() {
        if (otel_span) {
            otel_span->End();  // Auto-end on destruction
        }
    }
};

// Constructor
Span::Span(Impl* impl) : m_impl(impl) {}

// Destructor
Span::~Span() = default;

// Move constructor
Span::Span(Span&&) noexcept = default;

// Move assignment
Span& Span::operator=(Span&&) noexcept = default;

// Attributes
Span& Span::attr(std::string_view key, std::string_view value) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->SetAttribute(std::string(key), std::string(value));
    }
    return *this;
}

Span& Span::attr(std::string_view key, int64_t value) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->SetAttribute(std::string(key), value);
    }
    return *this;
}

Span& Span::attr(std::string_view key, double value) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->SetAttribute(std::string(key), value);
    }
    return *this;
}

Span& Span::attr(std::string_view key, bool value) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->SetAttribute(std::string(key), value);
    }
    return *this;
}

// Status
Span& Span::set_status(StatusCode code, std::string_view message) {
    if (m_impl && m_impl->otel_span) {
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
        m_impl->otel_span->SetStatus(otel_code, std::string(message));
    }
    return *this;
}

// Kind (must be set at span creation, but we'll do best effort here)
Span& Span::kind(SpanKind kind) {
    // Note: OpenTelemetry requires SpanKind at span creation
    // This is best-effort; ideally kind() should be parameter to span()
    // For now, we'll add it as an attribute for visibility
    if (m_impl && m_impl->otel_span) {
        const char* kind_str = "internal";
        switch (kind) {
            case SpanKind::Internal: kind_str = "internal"; break;
            case SpanKind::Server: kind_str = "server"; break;
            case SpanKind::Client: kind_str = "client"; break;
            case SpanKind::Producer: kind_str = "producer"; break;
            case SpanKind::Consumer: kind_str = "consumer"; break;
        }
        m_impl->otel_span->SetAttribute("span.kind", kind_str);
    }
    return *this;
}

// Events
Span& Span::add_event(std::string_view name) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->AddEvent(std::string(name));
    }
    return *this;
}

Span& Span::add_event(std::string_view name, Attributes attrs) {
    if (m_impl && m_impl->otel_span) {
        // Convert attributes to OTel format
        std::map<std::string, opentelemetry::common::AttributeValue> otel_attrs;
        for (const auto& [key, value] : attrs) {
            otel_attrs[std::string(key)] = std::string(value);
        }
        m_impl->otel_span->AddEvent(std::string(name), otel_attrs);
    }
    return *this;
}

// Context extraction
obs::Context Span::context() const {
    if (!m_impl || !m_impl->otel_span) {
        return obs::Context{};
    }
    
    auto otel_ctx = m_impl->otel_span->GetContext();
    
    // Convert OTel context to our Context
    obs::Context ctx;
    
    // Extract trace_id (128-bit)
    auto trace_id_bytes = otel_ctx.trace_id().Id();
    std::memcpy(&ctx.trace_id.high, trace_id_bytes.data(), 8);
    std::memcpy(&ctx.trace_id.low, trace_id_bytes.data() + 8, 8);
    
    // Extract span_id (64-bit)
    auto span_id_bytes = otel_ctx.span_id().Id();
    std::memcpy(&ctx.span_id.value, span_id_bytes.data(), 8);
    
    // Extract trace flags
    ctx.trace_flags = otel_ctx.trace_flags().flags();
    
    return ctx;
}

// Recording check
bool Span::is_recording() const {
    if (m_impl && m_impl->otel_span) {
        return m_impl->otel_span->IsRecording();
    }
    return false;
}

// Span creation functions
Span span(std::string_view name) {
    // Get tracer from provider
    auto& provider = Provider::instance();
    auto& impl = provider.impl();
    
    auto tracer = impl.get_tracer();
    if (!tracer) {
        // Return invalid span if provider not initialized
        return Span{nullptr};
    }
    
    // Check if there's an active span (for auto-parenting)
    auto parent_ctx = impl.get_active_context();
    
    trace_api::StartSpanOptions options;
    if (parent_ctx.is_valid()) {
        // Auto-parent to active span
        options.parent = impl.context_to_otel(parent_ctx);
    }
    
    auto otel_span = tracer->StartSpan(std::string(name), options);
    
    // Push to active span stack
    Span span{new Span::Impl{otel_span}};
    impl.push_active_span(span.context());
    
    return span;
}

Span span(std::string_view name, const obs::Context& parent) {
    auto& provider = Provider::instance();
    auto& impl = provider.impl();
    
    auto tracer = impl.get_tracer();
    if (!tracer) {
        return Span{nullptr};
    }
    
    trace_api::StartSpanOptions options;
    if (parent.is_valid()) {
        options.parent = impl.context_to_otel(parent);
    }
    
    auto otel_span = tracer->StartSpan(std::string(name), options);
    
    Span span{new Span::Impl{otel_span}};
    impl.push_active_span(span.context());
    
    return span;
}

} // namespace obs::v2
```

### Step 3.3: Modify ProviderImpl for Tracing

**File:** `src/v2/ProviderImpl.h` (ADD these members)

```cpp
// Add these includes at top:
#include <Context.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <vector>

// Add these members to ProviderImpl class:
class ProviderImpl {
public:
    // ... existing members ...
    
    // NEW: Tracer access
    nostd::shared_ptr<trace_api::Tracer> get_tracer();
    
    // NEW: Active context management (thread-local stack)
    obs::Context get_active_context();
    void push_active_span(const obs::Context& ctx);
    void pop_active_span();
    
    // NEW: Context conversion helper
    trace_api::SpanContext context_to_otel(const obs::Context& ctx);
    
private:
    // ... existing members ...
    
    // NEW: Tracer provider and tracer
    nostd::shared_ptr<trace_api::TracerProvider> m_tracer_provider;
    nostd::shared_ptr<trace_api::Tracer> m_tracer;
    
    // NEW: Thread-local active span stack
    static thread_local std::vector<obs::Context> active_span_stack;
};
```

**File:** `src/v2/ProviderImpl.cpp` (ADD these implementations)

```cpp
// Add includes
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h>

namespace trace_sdk = opentelemetry::sdk::trace;

// Thread-local active span stack
thread_local std::vector<obs::Context> ProviderImpl::active_span_stack;

// In init() method, ADD tracer setup:
bool ProviderImpl::init(const Config& config) {
    if (m_initialized) return true;
    
    try {
        // ... existing OTLP metrics setup ...
        
        // NEW: Setup tracer
        otlp::OtlpGrpcExporterOptions trace_exporter_opts;
        trace_exporter_opts.endpoint = config.otlp_endpoint;
        
        auto trace_exporter = otlp::OtlpGrpcExporterFactory::Create(trace_exporter_opts);
        auto trace_processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(trace_exporter));
        
        m_tracer_provider = trace_sdk::TracerProviderFactory::Create(std::move(trace_processor), res);
        
        // Set global tracer provider
        trace_api::Provider::SetTracerProvider(m_tracer_provider);
        
        // Get tracer
        m_tracer = trace_api::Provider::GetTracerProvider()->GetTracer(
            config.service_name, config.service_version);
        
        m_initialized = true;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// NEW: Tracer access
nostd::shared_ptr<trace_api::Tracer> ProviderImpl::get_tracer() {
    return m_tracer;
}

// NEW: Active context management
obs::Context ProviderImpl::get_active_context() {
    if (!active_span_stack.empty()) {
        return active_span_stack.back();
    }
    return obs::Context{};
}

void ProviderImpl::push_active_span(const obs::Context& ctx) {
    active_span_stack.push_back(ctx);
}

void ProviderImpl::pop_active_span() {
    if (!active_span_stack.empty()) {
        active_span_stack.pop_back();
    }
}

// NEW: Context conversion
trace_api::SpanContext ProviderImpl::context_to_otel(const obs::Context& ctx) {
    // Convert our Context to OTel SpanContext
    std::array<uint8_t, 16> trace_id_bytes;
    std::memcpy(trace_id_bytes.data(), &ctx.trace_id.high, 8);
    std::memcpy(trace_id_bytes.data() + 8, &ctx.trace_id.low, 8);
    
    std::array<uint8_t, 8> span_id_bytes;
    std::memcpy(span_id_bytes.data(), &ctx.span_id.value, 8);
    
    return trace_api::SpanContext(
        trace_api::TraceId(trace_id_bytes),
        trace_api::SpanId(span_id_bytes),
        trace_api::TraceFlags(ctx.trace_flags),
        true  // is_remote = true for propagated contexts
    );
}
```

### Step 3.4: Fix Span Destructor to Pop Context

**In `src/v2/Span.cpp`, modify Span::Impl destructor:**

```cpp
struct Span::Impl {
    nostd::shared_ptr<trace_api::Span> otel_span;
    
    Impl(nostd::shared_ptr<trace_api::Span> span)
        : otel_span(std::move(span)) {}
    
    ~Impl() {
        if (otel_span) {
            otel_span->End();  // Auto-end span
            
            // Pop from active span stack
            auto& provider = Provider::instance();
            provider.impl().pop_active_span();
        }
    }
};
```

---

## Phase 4: Log Implementation

### Overview

Implement structured logging with:
- Log levels (Trace, Debug, Info, Warn, Error, Fatal)
- Attributes (key-value pairs)
- **CRITICAL:** Automatic trace correlation (auto-inject trace_id and span_id)
- Scoped attributes (MDC pattern)

### Step 4.1: Create Log Header

**File:** `include/obs/v2/Log.h`

```cpp
#pragma once
#include <string_view>
#include <initializer_list>
#include <utility>
#include <map>
#include <string>

namespace obs::v2 {

// Log levels (OpenTelemetry standard)
enum class Level {
    Trace = 1,   // TRACE
    Debug = 5,   // DEBUG
    Info = 9,    // INFO
    Warn = 13,   // WARN
    Error = 17,  // ERROR
    Fatal = 21   // FATAL
};

// Attributes for logs
using Attributes = std::initializer_list<std::pair<std::string_view, std::string_view>>;

// Core logging function
void log(Level level, std::string_view message, Attributes attrs = {});

// Convenience functions
inline void trace(std::string_view msg, Attributes attrs = {}) {
    log(Level::Trace, msg, attrs);
}

inline void debug(std::string_view msg, Attributes attrs = {}) {
    log(Level::Debug, msg, attrs);
}

inline void info(std::string_view msg, Attributes attrs = {}) {
    log(Level::Info, msg, attrs);
}

inline void warn(std::string_view msg, Attributes attrs = {}) {
    log(Level::Warn, msg, attrs);
}

inline void error(std::string_view msg, Attributes attrs = {}) {
    log(Level::Error, msg, attrs);
}

inline void fatal(std::string_view msg, Attributes attrs = {}) {
    log(Level::Fatal, msg, attrs);
}

/**
 * Scoped Log Attributes (MDC Pattern)
 * 
 * Usage:
 *   void handle_request(const Request& req) {
 *       ScopedLogAttributes scoped({
 *           {"request.id", req.id()},
 *           {"session.id", req.session_id()}
 *       });
 *       
 *       // ALL logs in this scope inherit these attributes
 *       obs::v2::info("Processing request");
 *       // Includes: request.id, session.id, trace_id, span_id
 *   } // Scoped attributes removed on destruction
 */
class ScopedLogAttributes {
public:
    explicit ScopedLogAttributes(Attributes attrs);
    ~ScopedLogAttributes();
    
    // Non-copyable, non-movable
    ScopedLogAttributes(const ScopedLogAttributes&) = delete;
    ScopedLogAttributes& operator=(const ScopedLogAttributes&) = delete;
    ScopedLogAttributes(ScopedLogAttributes&&) = delete;
    ScopedLogAttributes& operator=(ScopedLogAttributes&&) = delete;
    
private:
    size_t m_stack_size;  // For restoring stack on destruction
};

} // namespace obs::v2
```

### Step 4.2: Create Log Implementation

**File:** `src/v2/Log.cpp`

```cpp
#include <obs/v2/Log.h>
#include "ProviderImpl.h"
#include <obs/v2/Provider.h>

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/logs/logger.h>

namespace obs::v2 {

namespace logs_api = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

// Thread-local scoped attributes stack
thread_local std::vector<std::map<std::string, std::string>> scoped_attributes_stack;

// Core logging function
void log(Level level, std::string_view message, Attributes attrs) {
    auto& provider = Provider::instance();
    auto& impl = provider.impl();
    
    auto logger = impl.get_logger();
    if (!logger) {
        return;  // Provider not initialized
    }
    
    // Build attributes map
    std::map<std::string, opentelemetry::common::AttributeValue> otel_attrs;
    
    // 1. Add user-provided attributes
    for (const auto& [key, value] : attrs) {
        otel_attrs[std::string(key)] = std::string(value);
    }
    
    // 2. Add scoped attributes (from ScopedLogAttributes)
    for (const auto& scope_attrs : scoped_attributes_stack) {
        for (const auto& [key, value] : scope_attrs) {
            otel_attrs[key] = value;
        }
    }
    
    // 3. CRITICAL: Add automatic trace correlation
    auto active_ctx = impl.get_active_context();
    if (active_ctx.is_valid()) {
        otel_attrs["trace_id"] = active_ctx.trace_id.to_hex();
        otel_attrs["span_id"] = active_ctx.span_id.to_hex();
        otel_attrs["trace_flags"] = static_cast<int64_t>(active_ctx.trace_flags);
    }
    
    // Convert level
    logs_api::Severity otel_severity;
    switch (level) {
        case Level::Trace: otel_severity = logs_api::Severity::kTrace; break;
        case Level::Debug: otel_severity = logs_api::Severity::kDebug; break;
        case Level::Info:  otel_severity = logs_api::Severity::kInfo; break;
        case Level::Warn:  otel_severity = logs_api::Severity::kWarn; break;
        case Level::Error: otel_severity = logs_api::Severity::kError; break;
        case Level::Fatal: otel_severity = logs_api::Severity::kFatal; break;
        default:           otel_severity = logs_api::Severity::kInfo;
    }
    
    // Emit log
    logger->EmitLogRecord(otel_severity, std::string(message), otel_attrs);
}

// ScopedLogAttributes implementation
ScopedLogAttributes::ScopedLogAttributes(Attributes attrs) {
    std::map<std::string, std::string> attrs_map;
    for (const auto& [key, value] : attrs) {
        attrs_map[std::string(key)] = std::string(value);
    }
    
    scoped_attributes_stack.push_back(std::move(attrs_map));
    m_stack_size = scoped_attributes_stack.size();
}

ScopedLogAttributes::~ScopedLogAttributes() {
    // Pop our attributes from stack
    if (scoped_attributes_stack.size() == m_stack_size) {
        scoped_attributes_stack.pop_back();
    }
}

} // namespace obs::v2
```

### Step 4.3: Modify ProviderImpl for Logging

**File:** `src/v2/ProviderImpl.h` (ADD these members)

```cpp
// Add includes
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/logs/logger.h>

namespace logs_api = opentelemetry::logs;

// Add to ProviderImpl class:
class ProviderImpl {
public:
    // ... existing ...
    
    // NEW: Logger access
    nostd::shared_ptr<logs_api::Logger> get_logger();
    
private:
    // ... existing ...
    
    // NEW: Logger provider and logger
    nostd::shared_ptr<logs_api::LoggerProvider> m_logger_provider;
    nostd::shared_ptr<logs_api::Logger> m_logger;
};
```

**File:** `src/v2/ProviderImpl.cpp` (ADD these implementations)

```cpp
// Add includes
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_factory.h>

namespace logs_sdk = opentelemetry::sdk::logs;

// In init(), ADD logger setup:
bool ProviderImpl::init(const Config& config) {
    // ... existing metrics + tracing setup ...
    
    // NEW: Setup logger
    otlp::OtlpGrpcLogRecordExporterOptions log_exporter_opts;
    log_exporter_opts.endpoint = config.otlp_endpoint;
    
    auto log_exporter = otlp::OtlpGrpcLogRecordExporterFactory::Create(log_exporter_opts);
    auto log_processor = logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(log_exporter));
    
    m_logger_provider = logs_sdk::LoggerProviderFactory::Create(std::move(log_processor));
    
    // Set global logger provider
    logs_api::Provider::SetLoggerProvider(m_logger_provider);
    
    // Get logger
    m_logger = logs_api::Provider::GetLoggerProvider()->GetLogger(
        config.service_name, config.service_version);
    
    m_initialized = true;
    return true;
}

// NEW: Logger access
nostd::shared_ptr<logs_api::Logger> ProviderImpl::get_logger() {
    return m_logger;
}
```

---

## Phase 5: Cross-Cutting Concerns

### Step 5.1: Resource Attributes

**Modify:** `include/obs/v2/Config.h`

```cpp
// ADD this struct:
struct ResourceAttributes {
    // Service info
    std::string service_name;
    std::string service_version;
    std::string service_namespace;
    std::string service_instance_id;
    
    // Deployment info
    std::string deployment_environment;  // "production", "staging", "dev"
    
    // Cloud info (optional)
    std::string cloud_provider;  // "aws", "gcp", "azure"
    std::string cloud_region;
    std::string cloud_availability_zone;
    
    // Kubernetes info (optional)
    std::string k8s_cluster_name;
    std::string k8s_namespace;
    std::string k8s_pod_name;
    std::string k8s_node_name;
    
    // Host info (optional)
    std::string host_name;
    std::string host_id;
};

// MODIFY Config struct:
struct Config {
    std::string service_name;
    std::string service_version{"1.0.0"};
    std::string environment{"production"};
    std::string otlp_endpoint{"http://localhost:4317"};
    bool enable_metrics{true};
    bool enable_tracing{true};
    bool enable_logging{true};
    
    // NEW: Optional resource attributes (overrides defaults)
    ResourceAttributes resource_attrs;
};
```

**Modify:** `src/v2/ProviderImpl.cpp`

```cpp
// In init(), use resource_attrs from config if provided:
bool ProviderImpl::init(const Config& config) {
    if (m_initialized) return true;
    
    try {
        // Build resource attributes
        resource::ResourceAttributes resource_attrs;
        
        if (!config.resource_attrs.service_name.empty()) {
            resource_attrs["service.name"] = config.resource_attrs.service_name;
        } else {
            resource_attrs["service.name"] = config.service_name;
        }
        
        if (!config.resource_attrs.service_version.empty()) {
            resource_attrs["service.version"] = config.resource_attrs.service_version;
        } else {
            resource_attrs["service.version"] = config.service_version;
        }
        
        if (!config.resource_attrs.deployment_environment.empty()) {
            resource_attrs["deployment.environment"] = config.resource_attrs.deployment_environment;
        } else {
            resource_attrs["deployment.environment"] = config.environment;
        }
        
        // Add optional cloud attributes
        if (!config.resource_attrs.cloud_provider.empty()) {
            resource_attrs["cloud.provider"] = config.resource_attrs.cloud_provider;
        }
        if (!config.resource_attrs.cloud_region.empty()) {
            resource_attrs["cloud.region"] = config.resource_attrs.cloud_region;
        }
        
        // Add optional k8s attributes
        if (!config.resource_attrs.k8s_cluster_name.empty()) {
            resource_attrs["k8s.cluster.name"] = config.resource_attrs.k8s_cluster_name;
        }
        if (!config.resource_attrs.k8s_namespace.empty()) {
            resource_attrs["k8s.namespace.name"] = config.resource_attrs.k8s_namespace;
        }
        if (!config.resource_attrs.k8s_pod_name.empty()) {
            resource_attrs["k8s.pod.name"] = config.resource_attrs.k8s_pod_name;
        }
        
        auto res = resource::Resource::Create(resource_attrs);
        
        // Use this res for all providers (metrics, traces, logs)
        // ... rest of init ...
    }
}
```

### Step 5.2: Graceful Shutdown with Flush

**Modify:** `src/v2/ProviderImpl.cpp`

```cpp
// Update shutdown() to flush all providers:
bool ProviderImpl::shutdown() {
    if (!m_initialized) return true;
    
    bool success = true;
    
    try {
        // Flush metrics
        if (m_meter_provider) {
            success &= m_meter_provider->ForceFlush(std::chrono::seconds(30));
        }
        
        // Flush traces
        if (m_tracer_provider) {
            success &= m_tracer_provider->ForceFlush(std::chrono::seconds(30));
        }
        
        // Flush logs
        if (m_logger_provider) {
            success &= m_logger_provider->ForceFlush(std::chrono::seconds(30));
        }
        
        m_initialized = false;
        return success;
    } catch (const std::exception&) {
        return false;
    }
}
```

---

## Critical Fixes

### Fix #1: DurationHistogram Attributes Support

**Already implemented** - See current `include/obs/v2/Metrics.h:66-71`

### Fix #2-6: From V2 Critical Fixes Document

These are already addressed in the current v2 implementation, but verify:

1. **Bounds checking** - MAX_METRICS limit with sentinel return
2. **Cache size limit** - Thread-local cache bounded to MAX_CACHE_SIZE
3. **OTLP exporter** - Using OtlpGrpcMetricExporter (not OStream)
4. **Error handling** - `init()` returns `bool`
5. **TLS cleanup** - Handled in shutdown()

---

## Testing

### Unit Tests to Create

#### `test/v2/span_test.cpp`

```cpp
#include <obs/v2/Span.h>
#include <obs/v2/Provider.h>
#include <gtest/gtest.h>

class SpanTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::v2::Config config{.service_name = "test-service"};
        obs::v2::init(config);
    }
    
    void TearDown() override {
        obs::v2::shutdown();
    }
};

TEST_F(SpanTest, BasicSpanCreation) {
    {
        auto span = obs::v2::span("test.operation");
        EXPECT_TRUE(span.is_recording());
    }
    // Span should auto-end on destruction
}

TEST_F(SpanTest, SpanAttributes) {
    auto span = obs::v2::span("test.operation");
    
    EXPECT_NO_THROW(span.attr("string_key", "value"));
    EXPECT_NO_THROW(span.attr("int_ke", 42));
    EXPECT_NO_THROW(span.attr("double_key", 3.14));
    EXPECT_NO_THROW(span.attr("bool_key", true));
}

TEST_F(SpanTest, SpanStatus) {
    auto span = obs::v2::span("test.operation");
    
    EXPECT_NO_THROW(span.set_status(obs::v2::StatusCode::Ok));
    EXPECT_NO_THROW(span.set_status(obs::v2::StatusCode::Error, "failed"));
}

TEST_F(SpanTest, SpanKind) {
    auto span = obs::v2::span("test.operation");
    
    EXPECT_NO_THROW(span.kind(obs::v2::SpanKind::Server));
}

TEST_F(SpanTest, SpanEvents) {
    auto span = obs::v2::span("test.operation");
    
    EXPECT_NO_THROW(span.add_event("event1"));
    EXPECT_NO_THROW(span.add_event("event2", {{"key", "value"}}));
}

TEST_F(SpanTest, SpanContextPropagation) {
    auto parent = obs::v2::span("parent");
    auto parent_ctx = parent.context();
    
    EXPECT_TRUE(parent_ctx.is_valid());
    EXPECT_TRUE(parent_ctx.trace_id.is_valid());
    EXPECT_TRUE(parent_ctx.span_id.is_valid());
}

TEST_F(SpanTest, AutoParenting) {
    {
        auto parent = obs::v2::span("parent");
        auto parent_ctx = parent.context();
        
        {
            auto child = obs::v2::span("child");
            auto child_ctx = child.context();
            
            // Child should have same trace_id as parent
            EXPECT_EQ(child_ctx.trace_id.high, parent_ctx.trace_id.high);
            EXPECT_EQ(child_ctx.trace_id.low, parent_ctx.trace_id.low);
            
            // Child should have different span_id
            EXPECT_NE(child_ctx.span_id.value, parent_ctx.span_id.value);
        }
    }
}

TEST_F(SpanTest, MoveSemantics) {
    auto span1 = obs::v2::span("test");
    auto span2 = std::move(span1);
    
    EXPECT_TRUE(span2.is_recording());
}
```

#### `test/v2/log_test.cpp`

```cpp
#include <obs/v2/Log.h>
#include <obs/v2/Span.h>
#include <obs/v2/Provider.h>
#include <gtest/gtest.h>

class LogTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::v2::Config config{.service_name = "test-service"};
        obs::v2::init(config);
    }
    
    void TearDown() override {
        obs::v2::shutdown();
    }
};

TEST_F(LogTest, BasicLogging) {
    EXPECT_NO_THROW(obs::v2::trace("trace message"));
    EXPECT_NO_THROW(obs::v2::debug("debug message"));
    EXPECT_NO_THROW(obs::v2::info("info message"));
    EXPECT_NO_THROW(obs::v2::warn("warn message"));
    EXPECT_NO_THROW(obs::v2::error("error message"));
    EXPECT_NO_THROW(obs::v2::fatal("fatal message"));
}

TEST_F(LogTest, LogWithAttributes) {
    EXPECT_NO_THROW(obs::v2::info("test", {
        {"key1", "value1"},
        {"key2", "value2"}
    }));
}

TEST_F(LogTest, ScopedAttributes) {
    {
        obs::v2::ScopedLogAttributes scoped({
            {"request.id", "req-123"},
            {"session.id", "sess-456"}
        });
        
        // Logs in this scope should inherit scoped attributes
        EXPECT_NO_THROW(obs::v2::info("message with scoped attrs"));
    }
    
    // Scoped attributes should be removed now
    EXPECT_NO_THROW(obs::v2::info("message without scoped attrs"));
}

TEST_F(LogTest, AutomaticTraceCorrelation) {
    {
        auto span = obs::v2::span("test.operation");
        auto ctx = span.context();
        
        // Log within span should automatically include trace_id and span_id
        EXPECT_NO_THROW(obs::v2::info("log with trace correlation"));
        
        // Note: Actual verification would require capturing log output
        // and checking for trace_id/span_id attributes
    }
}

TEST_F(LogTest, NestedScopedAttributes) {
    {
        obs::v2::ScopedLogAttributes scope1({{"key1", "value1"}});
        
        {
            obs::v2::ScopedLogAttributes scope2({{"key2", "value2"}});
            
            // Should have both scope1 and scope2 attributes
            EXPECT_NO_THROW(obs::v2::info("nested scoped log"));
        }
        
        // Should only have scope1 attributes
        EXPECT_NO_THROW(obs::v2::info("outer scoped log"));
    }
}
```

#### `test/v2/integration_test.cpp`

```cpp
#include <obs/v2/Span.h>
#include <obs/v2/Log.h>
#include <obs/v2/Metrics.h>
#include <obs/v2/Provider.h>
#include <gtest/gtest.h>
#include <chrono>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::v2::Config config{
            .service_name = "integration-test",
            .service_version = "1.0.0"
        };
        obs::v2::init(config);
    }
    
    void TearDown() override {
        obs::v2::shutdown();
    }
};

TEST_F(IntegrationTest, FullObservabilityStack) {
    // Simulate a complete request with metrics, tracing, and logging
    
    auto request_counter = obs::v2::register_counter("test.requests");
    auto latency_hist = obs::v2::register_duration_histogram("test.latency");
    
    {
        obs::v2::ScopedLogAttributes scoped({
            {"request.id", "req-12345"},
            {"client.ip", "192.168.1.1"}
        });
        
        auto span = obs::v2::span("handle_request");
        span.kind(obs::v2::SpanKind::Server);
        span.attr("http.method", "GET");
        span.attr("http.route", "/api/test");
        
        auto start = std::chrono::steady_clock::now();
        
        obs::v2::info("Request started");
        request_counter.inc();
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        span.add_event("processing_started");
        
        {
            auto db_span = obs::v2::span("database_query");
            db_span.kind(obs::v2::SpanKind::Client);
            db_span.attr("db.system", "postgresql");
            
            obs::v2::debug("Executing database query");
            
            // Simulate DB work
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            
            db_span.set_status(obs::v2::StatusCode::Ok);
        }
        
        auto duration = std::chrono::steady_clock::now() - start;
        latency_hist.record(duration);
        
        span.set_status(obs::v2::StatusCode::Ok);
        obs::v2::info("Request completed", {
            {"duration_ms", std::to_string(
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
            )}
        });
    }
    
    // All spans auto-ended, metrics recorded, logs emitted
}

TEST_F(IntegrationTest, ErrorHandling) {
    auto error_counter = obs::v2::register_counter("test.errors");
    
    {
        auto span = obs::v2::span("failing_operation");
        
        try {
            obs::v2::warn("About to fail");
            throw std::runtime_error("Simulated error");
        } catch (const std::exception& e) {
            error_counter.inc();
            span.set_status(obs::v2::StatusCode::Error, e.what());
            obs::v2::error("Operation failed", {
                {"error.type", "runtime_error"},
                {"error.message", e.what()}
            });
        }
    }
}
```

### Test Execution

```bash
# Build tests
docker exec astra bash -c "cd /app/astra && cmake --build build/gcc-debug --target test_observability -j2"

# Run all v2 tests
docker exec astra bash -c "cd /app/astra && ./build/gcc-debug/bin/test_observability --gtest_filter='*V2*'"

# Run specific test suites
docker exec astra bash -c "cd /app/astra && ./build/gcc-debug/bin/test_observability --gtest_filter='SpanTest.*'"
docker exec astra bash -c "cd /app/astra && ./build/gcc-debug/bin/test_observability --gtest_filter='LogTest.*'"
docker exec astra bash -c "cd /app/astra && ./build/gcc-debug/bin/test_observability --gtest_filter='IntegrationTest.*'"

# Run with ThreadSanitizer
docker exec astra bash -c "cd /app/astra && cmake --build build/gcc-tsan --target test_observability -j2"
docker exec astra bash -c "cd /app/astra && ./build/gcc-tsan/bin/test_observability --gtest_filter='*V2*'"
```

---

## API Usage Examples

### Example 1: Simple HTTP Request Handler (MetricsRegistry Pattern)

```cpp
#include <obs/v2/Span.h>
#include <obs/v2/Log.h>
#include <obs/v2/MetricsRegistry.h>

class HttpHandler {
public:
    HttpHandler() {
        // ✅ Register ALL metrics in constructor using fluent API
        // ONE member variable instead of 100!
        m_metrics
            .counter("requests", "http.requests.total")
            .counter("errors", "http.errors.total")
            .counter("success", "http.requests.success")
            .duration_histogram("latency", "http.request.latency")
            .histogram("payload_size", "http.payload.bytes", obs::v2::Unit::Bytes)
            .gauge("active", "http.active_requests");
    }
    
    void handle(const Request& req, Response& res) {
        // Create span (auto-ends on scope exit)
        // ✅ Value type - use span.attr() NOT span->attr()
        auto span = obs::v2::span("http.handle_request");
        span.kind(obs::v2::SpanKind::Server);
        span.attr("http.method", req.method());
        span.attr("http.route", req.route());
        
        // Scoped attributes for all logs in this request
        obs::v2::ScopedLogAttributes scoped({
            {"request.id", req.id()},
            {"client.ip", req.client_ip()}
        });
        
        auto start = std::chrono::steady_clock::now();
        
        obs::v2::info("Request received");
        
        // ✅ Use short keys to access registered metrics (fast map lookup)
        m_metrics.counter("requests").inc();
        m_metrics.gauge("active").add(1);
        
        try {
            // Process request
            process(req, res);
            
            m_metrics.counter("success").inc();
            span.set_status(obs::v2::StatusCode::Ok);
            obs::v2::info("Request completed");
            
        } catch (const std::exception& e) {
            m_metrics.counter("errors").inc();
            span.set_status(obs::v2::StatusCode::Error, e.what());
            obs::v2::error("Request failed", {
                {"error.message", e.what()}
            });
            res.status(500);
        }
        
        auto duration = std::chrono::steady_clock::now() - start;
        
        // ✅ Duration histogram accepts chrono::duration directly
        m_metrics.duration_histogram("latency").record(duration);
        m_metrics.histogram("payload_size").record(static_cast<double>(res.body().size()));
        m_metrics.gauge("active").add(-1);
    }
    
private:
    // ✅ ONE member variable for ALL metrics!
    obs::v2::MetricsRegistry m_metrics;
};
```

### Example 2: Repository Decorator Pattern (MetricsRegistry - Recommended)

```cpp
#include <obs/v2/MetricsRegistry.h>
#include <obs/v2/Span.h>
#include <obs/v2/Log.h>

class ObservableLinkRepository : public ILinkRepository {
public:
    ObservableLinkRepository(std::unique_ptr<ILinkRepository> inner)
        : m_inner(std::move(inner))
    {
        // ✅ Register ALL metrics in one place - clean, fluent API
        m_metrics
            .counter("save.attempts", "link.save.attempts")
            .counter("save.success", "link.save.success")
            .counter("save.errors", "link.save.errors")
            .counter("find.attempts", "link.find.attempts")
            .counter("find.hits", "link.find.hits")
            .counter("find.misses", "link.find.misses")
            .duration_histogram("save.latency", "link.save.latency")
            .duration_histogram("find.latency", "link.find.latency")
            .gauge("cache.size", "link.cache.size");
        // If you had 100 metrics, it's still just ONE member variable!
    }
    
    Result<void> save(const Link& link) override {
        // ✅ Value-type span (NOT pointer)
        auto span = obs::v2::span("link.save");
        span.kind(obs::v2::SpanKind::Internal);
        span.attr("link.code", link.code());
        
        auto start = std::chrono::steady_clock::now();
        
        // ✅ Short key lookup - fast!
        m_metrics.counter("save.attempts").inc();
        
        obs::v2::debug("Saving link", {
            {"code", link.code()},
            {"url", link.url()}
        });
        
        auto result = m_inner->save(link);
        
        auto duration = std::chrono::steady_clock::now() - start;
        m_metrics.duration_histogram("save.latency").record(duration);
        
        if (result.is_error()) {
            m_metrics.counter("save.errors").inc();
            span.set_status(obs::v2::StatusCode::Error, result.error().message());
            obs::v2::error("Failed to save link", {
                {"error", result.error().message()}
            });
        } else {
            m_metrics.counter("save.success").inc();
            span.set_status(obs::v2::StatusCode::Ok);
            obs::v2::info("Link saved successfully");
        }
        
        return result;
    }
    
    Result<Link> find_by_code(const ShortCode& code) override {
        auto span = obs::v2::span("link.find");
        span.attr("code", code.value());
        
        m_metrics.counter("find.attempts").inc();
        
        auto start = std::chrono::steady_clock::now();
        auto result = m_inner->find_by_code(code);
        auto duration = std::chrono::steady_clock::now() - start;
        
        m_metrics.duration_histogram("find.latency").record(duration);
        
        if (result.is_ok()) {
            m_metrics.counter("find.hits").inc();
            span.set_status(obs::v2::StatusCode::Ok);
        } else {
            m_metrics.counter("find.misses").inc();
            span.set_status(obs::v2::StatusCode::Error, "Not found");
        }
        
        return result;
    }
    
private:
    std::unique_ptr<ILinkRepository> m_inner;
    // ✅ ONE member variable for ALL metrics!
    obs::v2::MetricsRegistry m_metrics;
};
```

### Example 3: Application Initialization

```cpp
#include <obs/v2/Provider.h>

int main() {
    // Initialize observability
    obs::v2::Config obs_config{
        .service_name = "uri-shortener",
        .service_version = "1.0.0",
        .environment = "production",
        .otlp_endpoint = "http://otel-collector:4317"
    };
    
    // Optional: Add resource attributes
    obs_config.resource_attrs = {
        .cloud_provider = "aws",
        .cloud_region = "us-east-1",
        .k8s_cluster_name = "prod-cluster",
        .k8s_namespace = "default",
        .k8s_pod_name = get_pod_name()
    };
    
    if (!obs::v2::init(obs_config)) {
        std::cerr << "Failed to initialize observability\n";
        return 1;
    }
    
    obs::v2::info("Application starting", {
        {"version", obs_config.service_version},
        {"environment", obs_config.environment}
    });
    
    try {
        // Run application
        run_application();
    } catch (const std::exception& e) {
        obs::v2::fatal("Application crashed", {
            {"error", e.what()}
        });
    }
    
    obs::v2::info("Application shutting down");
    
    // Graceful shutdown with flush
    if (!obs::v2::shutdown()) {
        std::cerr << "Failed to flush observability data\n";
    }
    
    return 0;
}
```

---

## CMakeLists.txt Updates

**Update:** `libs/core/observability/CMakeLists.txt`

```cmake
# Add new source files
target_sources(observability
    PRIVATE
        # Existing
        src/v2/Provider.cpp
        src/v2/ProviderImpl.cpp
        src/v2/Metrics.cpp
        src/v2/MetricsRegistry.cpp
        src/v2/Context.cpp
        # NEW
        src/v2/Span.cpp
        src/v2/Log.cpp
)

# Add new header files
target_sources(observability
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            # Existing
            include/obs/v2/Config.h
            include/obs/v2/Provider.h
            include/obs/v2/Metrics.h
            include/obs/v2/MetricsRegistry.h
            include/obs/v2/Context.h
            # NEW
            include/obs/v2/Span.h
            include/obs/v2/Log.h
)

# Add new dependencies
target_link_libraries(observability
    PRIVATE
        # Existing
        opentelemetry-cpp::metrics
        opentelemetry-cpp::otlp_grpc_metrics_exporter
        # NEW
        opentelemetry-cpp::trace
        opentelemetry-cpp::otlp_grpc_exporter  # For traces
        opentelemetry-cpp::logs
        opentelemetry-cpp::otlp_grpc_log_record_exporter
)

# Add new test files
add_executable(test_observability
    # Existing tests
    test/v2/provider_test.cpp
    test/v2/metrics_test.cpp
    test/v2/metrics_registry_test.cpp
    test/v2/attributes_test.cpp
    test/v2/safety_test.cpp
    # NEW tests
    test/v2/span_test.cpp
    test/v2/log_test.cpp
    test/v2/integration_test.cpp
)
```

---

## Success Criteria

- [ ] All existing tests pass (44 tests)
- [ ] New Span tests pass (6+ tests in `span_test.cpp`)
- [ ] New Log tests pass (5+ tests in `log_test.cpp`)
- [ ] Integration test passes (full stack verification)
- [ ] TSan clean (no data races)
- [ ] Zero compiler warnings
- [ ] Manual verification: Logs include trace_id and span_id
- [ ] Manual verification: OTLP collector receives all three signals (metrics, traces, logs)

---

## Notes

- **Breaking Changes:** None - this adds new functionality in `v2` namespace
- **Performance:** Maintains <15 cycle target for hot path operations
- **Thread Safety:** All operations are thread-safe via thread-local storage
- **OTLP Collector:** Requires running OTLP collector for E2E verification
- **Legacy Code:** Old `obs::` namespace remains untouched for backward compatibility

---

## References

- OpenTelemetry C++ API: https://opentelemetry.io/docs/instrumentation/cpp/
- W3C TraceContext: https://www.w3.org/TR/trace-context/
- OpenTelemetry Semantic Conventions: https://opentelemetry.io/docs/specs/semconv/

---

**End of Document**
