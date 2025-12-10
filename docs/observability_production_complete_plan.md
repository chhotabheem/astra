# Observability Library - Production-Complete Implementation Plan
**Industry Standard - Zero Compromises**

**Created:** 2025-12-09  
**Status:** Current implementation has 44/44 tests passing (Phases 1-2 complete)  
**Remaining:** Phases 3-5 with ALL industry-standard features

---

## 🎯 Gap Analysis Summary

### Current Status: 44 Tests Passing ✅
- ✅ Phase 1: Config + Provider (4 tests)
- ✅ Phase 2.1: Metrics Types (16 tests)
- ✅ Phase 2.2: OTel SDK Integration
- ✅ Phase 2.3: MetricsRegistry (8 tests)
- ✅ Fixes: Gauge delta, Attributes (6 tests)
- ✅ Safety: 10 concurrent/integration tests

### What's Missing (Industry Standard Requirements)

---

## 📊 METRICS - Gap Analysis

### ✅ Currently Implemented:
- Counter, Histogram, Gauge
- Attributes (labels/tags)
- Thread-local instruments (zero mutex)
- Type-safe units
- Registry pattern
- DEFAULT-CONSTRUCTED NULL HANDLES SAFE

### ❌ MISSING (P0 - MUST HAVE):
**None** - current metrics implementation is production-ready

### ❌ MISSING (P1 - SHOULD HAVE):

#### 1. Observable Gauges (Async Metrics)
```cpp
// Callback-based metrics for memory, connections, queue depth
auto obs_gauge = obs::v2::register_observable_gauge(
    "memory.heap_bytes",
    obs::v2::Unit::Bytes,
    []() -> int64_t {
        return get_current_heap_size();  // Called on export
    }
);

// Multiple callbacks
auto multi_gauge = obs::v2::register_observable_gauge(
    "thread_pool.size",
    obs::v2::Unit::Dimensionless,
    []() -> std::vector<Observation> {
        return {
            {{"state", "active"}, active_threads()},
            {{"state", "idle"}, idle_threads()},
            {{"state", "blocked"}, blocked_threads()}
        };
    }
);
```
**Why:** Memory, connections, queue depth need callbacks (can't be incremented)  
**Used by:** Prometheus, OTel, DataDog

#### 2. Metric Descriptions
```cpp
auto counter = obs::v2::register_counter(
    "http.requests.total",
    obs::v2::Unit::Dimensionless,
    {
        .description = "Total HTTP requests received",
        .help = "Incremented for each incoming HTTP request"
    }
);
```
**Why:** Self-documenting metrics, shows in Prometheus /metrics endpoint  
**Used by:** Prometheus, OTel

#### 3. Exemplars (Link Metrics to Traces)
```cpp
auto hist = obs::v2::register_histogram("request.latency");

// Record with trace context
hist.record(duration_ms, {
    {"endpoint", "/api/users"}
}, {
    .trace_id = span.context().trace_id,
    .span_id = span.context().span_id
});

// In Prometheus/Grafana: Click metric spike → Jump to trace!
```
**Why:** Jump from metric spike to specific traces  
**Used by:** OTel, Prometheus (new), Grafana

### ❌ MISSING (P2 - NICE TO HAVE):

#### 4. Summary/Percentiles
```cpp
auto summary = obs::v2::register_summary("request.latency");
summary.observe(duration);
// Automatically computes P50, P90, P95, P99
```
**Why:** SLA/SLO monitoring  
**Used by:** Prometheus Summary

#### 5. Delta Temporality Support
```cpp
obs::v2::Config config{
    .metrics_temporality = Temporality::Delta  // vs Cumulative
};
```
**Why:** Backend compatibility (DataDog needs Delta)  
**Used by:** OTel

---

## 📝 LOGS - Gap Analysis

### ✅ Currently Implemented:
- Basic log levels (Trace, Debug, Info, Warn, Error, Fatal)
- Simple string messages: `info("message")`

### ❌ MISSING (P0 - MUST HAVE):

#### 1. Structured Logging with Attributes

**Current (Insufficient):**
```cpp
obs::v2::info("User logged in");  // ❌ Unstructured string
```

**Industry Standard:**
```cpp
obs::v2::log(Level::Info, "User logged in", {
    {"user.id", "12345"},
    {"ip_address", "192.168.1.1"},
    {"session.id", "abc123"},
    {"login.method", "oauth"}
});

// Convenience wrappers
obs::v2::info("Request completed", {
    {"http.status_code", "200"},
    {"http.method", "GET"},
    {"response.size_bytes", "1024"}
});

obs::v2::error("Database error", {
    {"error.type", "timeout"},
    {"error.message", "Connection timed out after 30s"},
    {"db.host", "postgres-primary"},
    {"query.duration_ms", "30042"}
});
```

**Why:**
- ✅ Queryable: `WHERE http.status_code = 500`
- ✅ Aggregatable: `GROUP BY error.type`
- ✅ Filterable: `user.id = "12345"`

**Used by:** OpenTelemetry, Serilog, spdlog, ELK, DataDog, Honeycomb

#### 2. Automatic Trace Correlation

**CRITICAL:** Logs must automatically include `trace_id` and `span_id`

```cpp
void process_order(const Order& order) {
    auto span = obs::v2::span("process_order");
    span.attr("order.id", order.id());
    
    // Log AUTOMATICALLY includes trace_id and span_id!
    obs::v2::info("Processing order", {
        {"order.id", order.id()},
        {"items.count", std::to_string(order.items().size())}
    });
    
    // Exported as:
    // {
    //   "timestamp": "2025-12-09T22:00:00Z",
    //   "severity": "INFO",
    //   "body": "Processing order",
    //   "trace_id": "b7ad6b7169203331",     ← AUTO-ADDED
    //   "span_id": "67e9821b5e3a2b1f",      ← AUTO-ADDED
    //   "trace_flags": 1,                    ← AUTO-ADDED
    //   "attributes": {
    //     "order.id": "ORD-123",
    //     "items.count": "5"
    //   },
    //   "resource": {
    //     "service.name": "uri-shortener",
    //     "service.version": "1.0.0"
    //   }
    // }
    
    auto result = save_to_db(order);
    
    if (result.is_error()) {
        obs::v2::error("Database error", {
            {"error.code", result.error().code()}
        });
        // This log also has same trace_id/span_id
        // Click trace_id in logs → Jump to full trace!
    }
}
```

**Why:**
- See spike in error logs? Click `trace_id` → see full distributed trace
- See slow request in trace? Click `span_id` → see all logs from that operation
- **MANDATORY for distributed systems**

**Used by:** OTel, DataDog, Honeycomb, New Relic, Jaeger, Zipkin

#### 3. Scoped Attributes (MDC Pattern)

```cpp
void handle_request(const Request& req, Response& resp) {
    // Set request-scoped attributes
    obs::v2::ScopedLogAttributes scoped({
        {"request.id", req.id()},
        {"session.id", req.session_id()},
        {"client.ip", req.client_ip()},
        {"user.id", req.user_id()}
    });
    
    // ALL logs in this scope inherit these attributes!
    
    obs::v2::info("Validating request");
    // Includes: request.id, session.id, client.ip, user.id
    
    validate_input(req);
    // Any logs inside validate_input() also include these!
    
    obs::v2::info("Saving to database");
    // Includes: request.id, session.id, client.ip, user.id
    
    obs::v2::info("Request completed", {
        {"duration_ms", std::to_string(duration)}
    });
    // Includes: request.id, session.id, client.ip, user.id, duration_ms
    
} // Scoped attributes removed when destructed
```

**Why:**
- ✅ DRY: Don't repeat `request.id` in every log
- ✅ Consistent: Can't forget to add `request_id`
- ✅ Clean: Code focuses on logic, not boilerplate

**Used by:** Log4j MDC, spdlog contexts, OTel Baggage

### ❌ MISSING (P1 - SHOULD HAVE):

#### 4. Log Sampling
```cpp
// Debug logs expensive - sample them
obs::v2::log(Level::Debug, "Cache lookup", {
    {"key", key},
    {"found", found ? "true" : "false"}
}, {.sample_rate = 0.01});  // Only 1% logged

// Critical errors always logged
obs::v2::error("System failure", {
    {"component", "payment_gateway"}
});  // Never sampled
```
**Why:** High-volume systems need sampling  
**Used by:** OTel

---

## 🔍 TRACES - Gap Analysis

### ✅ Currently Implemented:
- W3C TraceContext (trace_id, span_id, trace_flags)
- Context propagation (traceparent header)
- Context parsing/serialization

### ❌ MISSING (P0 - MUST HAVE):

#### 1. Span Implementation (RAII Value Type)

```cpp
class Span {
public:
    ~Span();  // Auto-ends span
    
    // Move-only (no copy)
    Span(Span&&) noexcept;
    Span& operator=(Span&&) noexcept;
    Span(const Span&) = delete;
    Span& operator=(const Span&) = delete;
    
    // Fluent API - NO pointer syntax
    Span& attr(std::string_view key, std::string_view value);
    Span& attr(std::string_view key, int64_t value);
    Span& attr(std::string_view key, double value);
    Span& attr(std::string_view key, bool value);
    
    // P0 features
    Span& set_status(StatusCode code, std::string_view message = "");
    Span& kind(SpanKind kind);
    Span& add_event(std::string_view name, Attributes attrs = {});
    
    Context context() const;
    bool is_recording() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Span creation
Span span(std::string_view name);
Span span(std::string_view name, const Context& parent);
```

**Usage:**
```cpp
{
    auto span = obs::v2::span("database_query");
    span.kind(SpanKind::Client);
    span.attr("db.system", "postgresql");
    span.attr("db.statement", "SELECT * FROM users");
    
    auto result = execute_query();
    
    if (result.is_error()) {
        span.set_status(StatusCode::Error, "Query failed");
        span.add_event("retry_attempt", {{"count", "1"}});
    } else {
        span.set_status(StatusCode::Ok);
    }
    
} // Span auto-ends here (RAII)
```

#### 2. Span Status

```cpp
enum class StatusCode {
    Unset,   // Default
    Ok,      // Success
    Error    // Error occurred
};

span.set_status(StatusCode::Error, "Database timeout");
span.set_status(StatusCode::Ok);
```
**Why:** Error tracking in trace UIs  
**Used by:** OTel, Jaeger, Zipkin

#### 3. Span Kind

```cpp
enum class SpanKind {
    Internal,   // Default - internal operation
    Server,     // Server-side handler
    Client,     // Client-side call
    Producer,   // Message producer
    Consumer    // Message consumer
};

auto span = obs::v2::span("handle_request");
span.kind(SpanKind::Server);

auto span2 = obs::v2::span("http_call");
span2.kind(SpanKind::Client);
```
**Why:** Trace visualization (client/server pairing)  
**Used by:** OTel, Jaeger

#### 4. Span Events (Annotations)

```cpp
auto span = obs::v2::span("process_batch");
span.add_event("validation_started");
span.add_event("cache_miss", {{"key", "user:123"}});
span.add_event("retry_attempt", {
    {"count", "3"},
    {"delay_ms", "1000"}
});
span.add_event("fallback_triggered", {
    {"reason", "circuit_breaker_open"}
});
```
**Why:** Fine-grained timing within span  
**Used by:** OTel, Jaeger, Zipkin

#### 5. Active Span Context Management

```cpp
{
    auto parent = obs::v2::span("parent_operation");
    
    // Child spans automatically parent to active span
    {
        auto child1 = obs::v2::span("child1");
        // Auto-parents to parent_operation
        
        {
            auto grandchild = obs::v2::span("grandchild");
            // Auto-parents to child1
        }
        
        auto child2 = obs::v2::span("child2");
        // Auto-parents to parent_operation
    }
}
```
**Why:** Automatic span hierarchy, no manual context passing  
**Used by:** OTel Context API

### ❌ MISSING (P1 - SHOULD HAVE):

#### 6. Semantic Conventions

```cpp
// Standard attribute names (constants)
namespace obs::v2::semantic {
    // HTTP
    constexpr auto HTTP_METHOD = "http.method";
    constexpr auto HTTP_STATUS_CODE = "http.status_code";
    constexpr auto HTTP_ROUTE = "http.route";
    constexpr auto HTTP_TARGET = "http.target";
    
    // Database
    constexpr auto DB_SYSTEM = "db.system";
    constexpr auto DB_STATEMENT = "db.statement";
    constexpr auto DB_OPERATION = "db.operation";
    
    // Messaging
    constexpr auto MESSAGING_SYSTEM = "messaging.system";
    constexpr auto MESSAGING_OPERATION = "messaging.operation";
}

// Usage
span.attr(semantic::HTTP_METHOD, "GET");
span.attr(semantic::HTTP_STATUS_CODE, 200);
span.attr(semantic::DB_SYSTEM, "postgresql");
```
**Why:** Cross-platform compatibility  
**Used by:** OTel Semantic Conventions

#### 7. Span Links

```cpp
auto span1 = obs::v2::span("async_task_1");
auto ctx1 = span1.context();

auto span2 = obs::v2::span("async_task_2");
auto ctx2 = span2.context();

// Batch processor links to all tasks
auto batch = obs::v2::span("batch_processor", {
    .links = {
        {ctx1, {{"reason", "triggered_by"}}},
        {ctx2, {{"reason", "triggered_by"}}}
    }
});
```
**Why:** Fan-out/fan-in patterns, async workflows  
**Used by:** OTel

---

## 🌐 CROSS-CUTTING CONCERNS

### ❌ MISSING (P0 - MUST HAVE):

#### 1. Resource Attributes

```cpp
// In Config
struct Config {
    std::string service_name;
    std::string service_version;
    std::string environment;
    std::string otlp_endpoint;
    
    // NEW: Resource attributes
    ResourceAttributes resource = ResourceAttributes::create_default();
};

// Resource builder
ResourceAttributes resource = ResourceAttributes::builder()
    .service("uri-shortener", "1.0.0", "production")
    .instance_id("pod-xyz-123")
    .cloud("aws", "us-east-1")
    .kubernetes("uri-shortener-pod", "default", "node-1")
    .host("ip-10-0-1-42.ec2.internal")
    .build();

// Applied to ALL signals (metrics, traces, logs)
```
**Why:** Multi-service correlation, cloud-native metadata  
**Used by:** OTel

#### 2. Graceful Shutdown with Flush

```cpp
// Current (simple)
obs::v2::shutdown();

// Industry standard
bool success = obs::v2::shutdown({
    .flush_timeout = std::chrono::seconds(30),
    .force_flush = true
});

if (!success) {
    log_error("Failed to flush telemetry data");
}
```
**Why:** Don't lose data on shutdown  
**Used by:** OTel

### ❌ MISSING (P1 - SHOULD HAVE):

#### 3. Baggage (W3C)

```cpp
Context ctx = Context::create();
ctx.baggage.set("user_id", "12345");
ctx.baggage.set("tenant_id", "acme-corp");
ctx.baggage.set("correlation_id", "req-xyz");

// Propagates across service boundaries
// Child services can read:
std::string user_id = ctx.baggage.get("user_id");

// All child spans inherit baggage
auto span = obs::v2::span("operation", ctx);
span.attr("user.id", ctx.baggage.get("user_id"));
```
**Why:** User-defined context propagation  
**Used by:** W3C Baggage, OTel

#### 4. Multiple Propagators

```cpp
Config config{
    .propagators = {
        TraceContextPropagator(),  // W3C (default)
        B3Propagator(),            // Zipkin
        JaegerPropagator(),        // Jaeger
        BaggagePropagator()        // W3C Baggage
    }
};
```
**Why:** Cross-platform compatibility  
**Used by:** OTel

---

## 🎯 REAL-WORLD COMPLETE EXAMPLE

```cpp
// ============================================================================
// Complete Example: HTTP Request Handler with Full Observability
// ============================================================================
class UriShortenerHandler {
public:
    UriShortenerHandler() {
        // Setup metrics registry (once in constructor)
        m_metrics
            .counter("requests", "http.requests.total")
            .counter("errors", "http.errors.total")
            .histogram("latency", "http.request.latency_ms")
            .gauge("active_requests", "http.active_requests");
    }
    
    void handle(const HttpRequest& req, HttpResponse& resp) {
        // ----------------------------------------------------------------
        // 1. Create span (distributed tracing)
        // ----------------------------------------------------------------
        auto span = obs::v2::span("shorten_url");
        span.kind(SpanKind::Server);
        span.attr("http.method", req.method());
        span.attr("http.route", "/api/shorten");
        span.attr("http.target", req.uri());
        span.attr("client.ip", req.client_ip());
        
        // ----------------------------------------------------------------
        // 2. Scoped log attributes (auto-included in all logs)
        // ----------------------------------------------------------------
        obs::v2::ScopedLogAttributes scoped({
            {"request.id", req.id()},
            {"session.id", req.session_id()},
            {"client.ip", req.client_ip()}
        });
        // From here on, ALL logs include: request.id, session.id, client.ip
        // PLUS: trace_id and span_id (auto-added by correlation)
        
        // ----------------------------------------------------------------
        // 3. Metrics
        // ----------------------------------------------------------------
        m_metrics.counter("requests").inc(1, {
            {"endpoint", "/api/shorten"},
            {"method", "POST"}
        });
        
        m_metrics.gauge("active_requests").add(1);
        
        // ----------------------------------------------------------------
        // 4. Structured logging (with auto trace correlation)
        // ----------------------------------------------------------------
        obs::v2::info("Request received", {
            {"http.method", req.method()},
            {"url.length", std::to_string(req.body().size())}
        });
        // Exported as JSON:
        // {
        //   "timestamp": "2025-12-09T22:00:00Z",
        //   "severity": "INFO",
        //   "body": "Request received",
        //   "trace_id": "b7ad6b7169203331",      ← AUTO
        //   "span_id": "67e9821b5e3a2b1f",       ← AUTO
        //   "request.id": "req-123",              ← Scoped
        //   "session.id": "sess-456",             ← Scoped
        //   "client.ip": "192.168.1.1",           ← Scoped
        //   "http.method": "POST",
        //   "url.length": "45"
        // }
        
        // ----------------------------------------------------------------
        // 5. Timed operation with histogram
        // ----------------------------------------------------------------
        auto start = std::chrono::steady_clock::now();
        
        span.add_event("validation_started");
        auto result = shorten(req.body());
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        
        m_metrics.histogram("latency").record(duration.count(), {
            {"endpoint", "/api/shorten"}
        });
        
        // ----------------------------------------------------------------
        // 6. Error handling (logs + metrics + span status)
        // ----------------------------------------------------------------
        if (result.is_error()) {
            obs::v2::error("Shortening failed", {
                {"error.type", result.error().type()},
                {"error.message", result.error().message()},
                {"error.code", result.error().code()}
            });
            // Log includes: trace_id, span_id, request.id, session.id, etc.
            
            m_metrics.counter("errors").inc(1, {
                {"endpoint", "/api/shorten"},
                {"error.type", result.error().type()}
            });
            
            span.set_status(StatusCode::Error, result.error().message());
            span.attr("http.status_code", 500);
            span.add_event("error_occurred", {
                {"error.type", result.error().type()}
            });
            
            m_metrics.gauge("active_requests").add(-1);
            
            resp.status(500).json({{"error", result.error().message()}});
            return;
        }
        
        // ----------------------------------------------------------------
        // 7. Success path
        // ----------------------------------------------------------------
        obs::v2::info("URL shortened", {
            {"short_code", result->code()},
            {"target_url", result->url()},
            {"duration_ms", std::to_string(duration.count())}
        });
        
        span.attr("http.status_code", 200);
        span.attr("short_code", result->code());
        span.set_status(StatusCode::Ok);
        
        m_metrics.gauge("active_requests").add(-1);
        
        resp.status(200).json({{"code", result->code()}});
        
        // Span auto-ends here (RAII)
    }
    
private:
    obs::v2::MetricsRegistry m_metrics;
};
```

### What this gives you:

- **Metrics:** Request count, latency P50/P95/P99, error rate
- **Traces:** Full distributed trace with timing, attributes, status
- **Logs:** Structured, with automatic trace correlation + scoped attributes

### In your backend (DataDog/Honeycomb/Grafana):

- See spike in error metric? → Filter logs by `error.type` → Click `trace_id` → See full trace
- See slow request in P95 latency? → Jump to trace → See which span slow → See logs from that span
- Search logs by `request.id` → See all logs for that request → Click `trace_id` → See trace

---

## 📋 COMPLETE REVISED IMPLEMENTATION PLAN

### Phase 3: Tracing (4 hours)

#### Step 3.1: Context ✅ DONE
Already adapted to v2 namespace

#### Step 3.2: Span Implementation (3 hours)

**File:** `include/obs/v2/Span.h`
- RAII value type (move-only)
- Fluent `attr()` API
- Span Status (Ok/Error/Unset)
- Span Kind (Server/Client/Internal/Producer/Consumer)
- Span Events with attributes
- Context extraction

**File:** `src/v2/Span.cpp`
- OTel Tracer integration
- `Span::Impl` with OTel span
- Auto-end in destructor
- Event recording

**Tests:** `test/v2/span_test.cpp`
- RAII auto-end
- Move semantics
- Attributes, Status, Kind, Events
- Context propagation

#### Step 3.3: Active Context Management (1 hour)

**File:** `src/v2/ProviderImpl.h`
- Thread-local active span stack
- Auto-parenting logic

**Implementation:**
```cpp
// In ProviderImpl
thread_local std::vector<Context> active_span_stack;

Context get_active_context() {
    if (!active_span_stack.empty()) {
        return active_span_stack.back();
    }
    return Context{};  // No active span
}

void push_context(const Context& ctx) {
    active_span_stack.push_back(ctx);
}

void pop_context() {
    if (!active_span_stack.empty()) {
        active_span_stack.pop_back();
    }
}
```

---

### Phase 4: Logging (3 hours)

#### Step 4.1: Structured Logging API (1 hour)

**File:** `include/obs/v2/Log.h`

```cpp
enum class Level {
    Trace, Debug, Info, Warn, Error, Fatal
};

// Core API
void log(Level level, std::string_view message, Attributes attrs = {});

// Convenience
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
```

**File:** `src/v2/Log.cpp`
- OTel Logger integration
- Attribute conversion
- Level mapping

**Tests:** `test/v2/log_test.cpp`
- All levels
- Attributes
- Message formatting

#### Step 4.2: Automatic Trace Correlation (1 hour)

**Implementation:**
```cpp
void log(Level level, std::string_view message, Attributes attrs) {
    // Get active span context
    auto ctx = Provider::instance().impl().get_active_context();
    
    // Merge with provided attributes
    std::vector<std::pair<std::string, std::string>> merged_attrs;
    
    // Add trace context if present
    if (ctx.is_valid()) {
        merged_attrs.emplace_back("trace_id", ctx.trace_id.to_hex());
        merged_attrs.emplace_back("span_id", ctx.span_id.to_hex());
        merged_attrs.emplace_back("trace_flags", 
            std::to_string(ctx.trace_flags));
    }
    
    // Add user attributes
    for (const auto& [k, v] : attrs) {
        merged_attrs.emplace_back(std::string(k), std::string(v));
    }
    
    // Call OTel Logger
    // ... emit log record ...
}
```

**Tests:** `test/v2/log_correlation_test.cpp`
- Logs inside span include trace_id
- Logs outside span don't include trace_id
- Multiple nested spans

#### Step 4.3: Scoped Attributes (1 hour)

**File:** `include/obs/v2/Log.h` (add)

```cpp
class ScopedLogAttributes {
public:
    explicit ScopedLogAttributes(Attributes attrs);
    ~ScopedLogAttributes();
    
    ScopedLogAttributes(const ScopedLogAttributes&) = delete;
    ScopedLogAttributes& operator=(const ScopedLogAttributes&) = delete;
};
```

**Implementation:**
```cpp
// Thread-local attribute stack
thread_local std::vector<Attributes> scoped_attributes_stack;

ScopedLogAttributes::ScopedLogAttributes(Attributes attrs) {
    scoped_attributes_stack.push_back(attrs);
}

ScopedLogAttributes::~ScopedLogAttributes() {
    if (!scoped_attributes_stack.empty()) {
        scoped_attributes_stack.pop_back();
    }
}

// In log() function, merge all scoped attributes
void log(Level level, std::string_view message, Attributes attrs) {
    std::vector<std::pair<std::string, std::string>> merged;
    
    // 1. Add scoped attributes (bottom to top)
    for (const auto& scoped : scoped_attributes_stack) {
        for (const auto& [k, v] : scoped) {
            merged.emplace_back(std::string(k), std::string(v));
        }
    }
    
    // 2. Add trace context
    // ...
    
    // 3. Add user attributes (override scoped)
    for (const auto& [k, v] : attrs) {
        merged.emplace_back(std::string(k), std::string(v));
    }
    
    // Emit log
}
```

**Tests:** `test/v2/scoped_log_test.cpp`
- Single scope
- Nested scopes
- Scope auto-cleanup

---

### Phase 5: Resources & Config (1 hour)

#### Step 5.1: Resource Model

**File:** `include/obs/v2/Config.h` (update)

```cpp
struct ResourceAttributes {
    std::map<std::string, std::string> attributes;
    
    static ResourceAttributes create_default() {
        return ResourceAttributes{
            {{"service.name", "unknown"},
             {"service.version", "0.0.0"}}
        };
    }
    
    // Fluent builder
    class Builder {
    public:
        Builder& service(std::string_view name, 
                        std::string_view version,
                        std::string_view env) {
            attrs["service.name"] = std::string(name);
            attrs["service.version"] = std::string(version);
            attrs["deployment.environment"] = std::string(env);
            return *this;
        }
        
        Builder& instance_id(std::string_view id) {
            attrs["service.instance.id"] = std::string(id);
            return *this;
        }
        
        Builder& cloud(std::string_view provider, std::string_view region) {
            attrs["cloud.provider"] = std::string(provider);
            attrs["cloud.region"] = std::string(region);
            return *this;
        }
        
        Builder& kubernetes(std::string_view pod, 
                          std::string_view ns,
                          std::string_view node) {
            attrs["k8s.pod.name"] = std::string(pod);
            attrs["k8s.namespace.name"] = std::string(ns);
            attrs["k8s.node.name"] = std::string(node);
            return *this;
        }
        
        Builder& host(std::string_view hostname) {
            attrs["host.name"] = std::string(hostname);
            return *this;
        }
        
        ResourceAttributes build() {
            return ResourceAttributes{std::move(attrs)};
        }
        
    private:
        std::map<std::string, std::string> attrs;
    };
    
    static Builder builder() { return Builder{}; }
};

struct Config {
    std::string service_name;
    std::string service_version;
    std::string environment;
    std::string otlp_endpoint;
    
    // NEW
    ResourceAttributes resource = ResourceAttributes::create_default();
};
```

#### Step 5.2: Enhanced Shutdown

**File:** `include/obs/v2/Provider.h` (update)

```cpp
struct ShutdownOptions {
    std::chrono::milliseconds flush_timeout{30000};
    bool force_flush = true;
};

bool shutdown(const ShutdownOptions& opts = {});
```

---

## 📋 PRIORITY IMPLEMENTATION ORDER

### Implement NOW (P0 - Blocking for Production):
1. ✅ **Span (Status, Kind, Events)** - 3 hrs
2. ✅ **Active Context Management** - 1 hr
3. ✅ **Structured Logging** - 1 hr
4. ✅ **Trace Correlation** - 1 hr
5. ✅ **Scoped Attributes** - 1 hr
6. ✅ **Resource Model** - 1 hr

**Total: 8 hours to production-ready**

### Implement Later (P1 - Production Quality):
- Observable Gauges - 2 hrs
- Metric Descriptions - 0.5 hrs
- Exemplars - 1.5 hrs
- Semantic Conventions - 1 hr
- Span Links - 1 hr
- Baggage - 1.5 hrs
- Multiple Propagators - 1 hr
- Log Sampling - 0.5 hrs

**Total: 9 hours**

### Defer (P2 - Advanced):
- Summary/Percentiles
- Delta Temporality
- Custom Sampling

---

## 🎯 SUCCESS CRITERIA

### Metrics: ✅ COMPLETE
- ✅ 44 tests passing
- ✅ Thread-local instruments
- ✅ Attributes support
- ✅ Safety validated

### Traces: After Phase 3
- ✅ Span RAII auto-end
- ✅ Status, Kind, Events
- ✅ Active context
- ✅ Context propagation
- ✅ 20+ span tests

### Logs: After Phase 4
- ✅ Structured attributes
- ✅ Auto trace correlation
- ✅ Scoped attributes
- ✅ 15+ log tests

### Integration: After Phase 5
- ✅ Resource applied to all signals
- ✅ Graceful shutdown
- ✅ End-to-end test

---

## 📊 FINAL VALIDATION

Before declaring production-ready:

1. Run all tests (target: 80+ tests)
2. TSan validation (no data races)
3. Integration test (metrics + traces + logs together)
4. Real app migration (uri_shortener)
5. Performance validation (metrics <15 cycles)

---

**This document is the complete implementation guide. Nothing is omitted.**
