# Observability Library - Design Document

> **Status**: DESIGN COMPLETE - Ready for Implementation  
> **Last Updated**: 2025-12-05  
> **C++ Standard**: C++17 only

---

## 1. Design Principles

1. **Clean API** - `obs::span()`, `obs::info()`, `obs::counter()` - no verbose calls
2. **OTel Hidden** - OpenTelemetry is implementation detail, never exposed to other layers
3. **SEDA-aware** - `obs::Context` flows with `Job` across thread boundaries
4. **Testable** - `IBackend` interface enables mock injection for unit tests
5. **Thread-safe** - All public APIs are safe for concurrent use

---

## 2. Directory Structure

```
observability/
├── include/obs/                    # PUBLIC (no OTel includes here)
│   ├── Observability.h             # Main include + init/shutdown/set_backend
│   ├── Context.h                   # TraceId, SpanId, Baggage (flows with Job)
│   ├── Span.h                      # RAII span class
│   ├── Log.h                       # trace/debug/info/warn/error/fatal
│   ├── Metrics.h                   # Counter, Histogram classes
│   └── IBackend.h                  # Backend interface for DI
│
├── src/                            # PRIVATE (OTel hidden here)
│   ├── otel/OTelBackend.h/.cpp     # OpenTelemetry adapter
│   ├── noop/NoopBackend.cpp        # No-op implementation
│   ├── Context.cpp
│   ├── Span.cpp
│   └── Observability.cpp
│
└── test/
    ├── MockBackend.h               # Test mock
    ├── test_context.cpp
    ├── test_span.cpp
    └── test_integration.cpp
```

---

## 3. Core Concepts

### 3.1 Context (flows with Job in SEDA)

`obs::Context` contains trace ID, span ID, and baggage. It **flows with every Job** through the SEDA pipeline.

```cpp
// In concurrency/Job.h
struct Job {
    JobType type;
    uint64_t session_id;
    std::any payload;
    obs::Context trace_ctx;   // <-- Context propagates here
};
```

### 3.2 Backend Interface (Dependency Injection)

`IBackend` is set once at process boot. Enables:
- **Production**: `OTelBackend` (OpenTelemetry)
- **Tests**: `MockBackend` or `NoopBackend`
- **Future**: `DatadogBackend`, `JaegerBackend`, etc.

---

## 4. Public API

### 4.1 Initialization (main.cpp)

```cpp
#include <obs/Observability.h>

int main() {
    // Set backend at boot
    obs::set_backend(std::make_unique<obs::OTelBackend>(
        obs::Config{
            .service_name = "my-service",
            .service_version = "1.0.0",
            .environment = "prod"
        }
    ));
    
    // ... run application ...
    
    obs::shutdown();
}
```

### 4.2 Tracing (RAII Span)

```cpp
void handle_request(Job& job) {
    // Span inherits context from Job, auto-ends on scope exit
    auto span = obs::span("handle_request", job.trace_ctx);
    span.attr("session_id", job.session_id);
    
    // Child job propagates context
    auto child_job = Job{
        .type = JobType::DB_QUERY,
        .session_id = job.session_id,
        .payload = query,
        .trace_ctx = span.context()  // Context flows to child
    };
    io_pool.submit(child_job);
}
```

### 4.3 Logging with Trace Correlation

```cpp
obs::info("Request received", job.trace_ctx);
obs::error("Failed to process", job.trace_ctx);
```

Logs automatically include trace_id and span_id from context.

### 4.4 Metrics

```cpp
obs::counter("http_requests_total").inc();
obs::histogram("request_latency_seconds").record(0.042);
```

### 4.5 ConsoleBackend (for debugging/tests)

Use ConsoleBackend to see all observability output on stderr:

```cpp
#include <obs/Observability.h>

int main() {
    // For debugging: use console backend (writes to stderr)
    obs::set_backend(std::make_unique<obs::ConsoleBackend>());
    
    auto span = obs::span("my_operation");
    obs::info("Processing request");
    obs::counter("requests").inc();
    
    obs::shutdown();
}
```

**Output on stderr:**
```
[OBS] ConsoleBackend initialized
[SPAN START] my_operation trace=a1b2c3d4
[INFO] Processing request trace=a1b2c3d4
[COUNTER] requests += 1 (total=1)
[SPAN END] my_operation duration=42us
[OBS] ConsoleBackend shutdown
```

---

## 5. HTTP Header Propagation (W3C Trace Context)

### Inbound (Network Thread)
```cpp
void on_request(HttpRequest& req) {
    // Parse W3C traceparent header
    auto ctx = obs::Context::from_traceparent(req.header("traceparent"));
    
    Job job{
        .type = JobType::HTTP_REQUEST,
        .session_id = generate_session_id(),
        .payload = req,
        .trace_ctx = ctx.is_valid() ? ctx : obs::Context::create()
    };
    worker_pool.submit(job);
}
```

### Outbound (IO Thread)
```cpp
void call_downstream(const Job& job) {
    auto span = obs::span("downstream_call", job.trace_ctx);
    
    http_client.set_header("traceparent", span.context().to_traceparent());
    http_client.post(url, body);
}
```

---

## 6. Testing Strategy

### 6.1 Unit Tests with MockBackend

```cpp
class MockBackend : public obs::IBackend {
public:
    std::vector<std::string> created_spans;
    
    std::unique_ptr<Span> create_span(std::string_view name, const Context&) override {
        created_spans.push_back(std::string(name));
        return std::make_unique<NoopSpan>();
    }
};

TEST(ObservabilityTest, SpanCreated) {
    auto mock = std::make_unique<MockBackend>();
    auto* ptr = mock.get();
    obs::set_backend(std::move(mock));
    
    { auto span = obs::span("test-span"); }
    
    EXPECT_EQ(ptr->created_spans.size(), 1);
    EXPECT_EQ(ptr->created_spans[0], "test-span");
}
```

### 6.2 Thread Safety Tests

Run with Helgrind:
```bash
valgrind --tool=helgrind ./build/gcc-debug/observability/test_observability
```

---

## 7. OTel Backend Implementation Notes

OTelBackend wraps OpenTelemetry SDK:
- Uses `TracerProvider`, `MeterProvider`, `LoggerProvider`
- Calls `ForceFlush()` and `Shutdown()` properly
- Thread-safe via internal mutex

This is the ONLY place OTel headers appear. Other layers never see OTel types.

---

## 8. Implementation Checklist

### Completed ✅
- [x] Context.h / Context.cpp
- [x] Span.h / Span.cpp
- [x] Log.h (header-only)
- [x] Metrics.h / Metrics.cpp
- [x] IBackend.h
- [x] OTelBackend.h / OTelBackend.cpp
- [x] Observability.h / Observability.cpp (includes set_backend)
- [x] ConsoleBackend.h (for debugging/tests - writes to stderr)
- [x] MockBackend.h (shared test mock in observability/test/)
- [x] Update CMakeLists.txt
- [x] Unit tests (test_context, test_span, test_log, test_metrics)
- [x] TSan validation (thread safety tests)

### ✅ All Items Complete
> **All design items have been implemented and tested with TSan**

- [x] **Job.trace_ctx field** - Added to `concurrency/Job.h` for SEDA context propagation
- [x] **NVI pattern for Span::attr(bool)** - Prevents string literals from matching bool overload
- [x] **test_console_backend.cpp** - Tests ConsoleBackend stderr output
- [x] **test_job_context.cpp** - Tests Job carries trace context
- [x] **test_seda_tracing.cpp** - Tests end-to-end SEDA context flow

### Job.trace_ctx Implementation
```cpp
// In concurrency/Job.h
struct Job {
    JobType type;
    uint64_t session_id;
    std::any payload;
    obs::Context trace_ctx;   // ✅ IMPLEMENTED - enables end-to-end tracing
};
```

---

## 9. Key Files Reference

| File | Purpose |
|:-----|:--------|
| `include/obs/Observability.h` | Main include, init/shutdown/set_backend |
| `include/obs/Context.h` | TraceId, SpanId, to_traceparent() |
| `include/obs/Span.h` | RAII span with attr(), set_error() |
| `include/obs/IBackend.h` | Interface for DI |
| `src/otel/OTelBackend.cpp` | OTel implementation (hidden) |
| `concurrency/Job.h` | Add `obs::Context trace_ctx` field |

---

**End of Document**
