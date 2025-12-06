# URI Shortener DDD - Complete ✅

**All 195 tests passing. HTTP/2 server functional.**

## Design Review Summary

### ✅ Implemented
- Domain layer isolation — Zero technology leakage
- Value object invariants — Self-validating at construction
- Use case pattern — Clean Input/Output structs
- Result-based error handling — No exceptions in domain
- Port interfaces — Pure abstractions

---

## Final Design Decisions

### 1. App Class with Factory Pattern (C++ Idiomatic)

**Follows Go/Rust industry pattern adapted for C++:**

```cpp
class UriShortenerApp {
public:
    // Factory - can fail gracefully
    [[nodiscard]] static astra::Result<UriShortenerApp, AppError> create(const Config& config);
    
    // Run the server (blocking), returns exit code
    [[nodiscard]] int run();
    
    // Moveable, not copyable
    UriShortenerApp(UriShortenerApp&&) = default;
    UriShortenerApp& operator=(UriShortenerApp&&) = default;
    
private:
    explicit UriShortenerApp(/* dependencies injected */);
    
    std::unique_ptr<IServer> m_server;
    std::shared_ptr<ILinkRepository> m_repo;
    std::shared_ptr<ICodeGenerator> m_gen;
    std::shared_ptr<ShortenLink> m_shorten;
    std::shared_ptr<ResolveLink> m_resolve;
    std::shared_ptr<DeleteLink> m_delete;
};
```

**main.cpp (6 lines):**
```cpp
int main() {
    auto result = UriShortenerApp::create(config::load());
    if (result.is_err()) {
        std::cerr << result.error() << "\n";
        return 1;
    }
    return result.value().run();
}
```

**Why this design:**
- Factory `create()` can validate and return errors
- Constructor receives ready-to-use dependencies (injected by factory)
- RAII — app owns resources, cleans up on destruction
- Move semantics — `result.value()` moves app out
- main() does nothing but call factory and run

### 2. Support Both HTTP/1.1 and HTTP/2
Forces proper abstraction. Both servers implement `router::IRequest/IResponse`.

### 3. Validation Responsibility

| Layer | Responsibility |
|-------|---------------|
| Router | Match routes, extract raw params |
| Controller | HTTP→Input transform, check param existence |
| Use Case | Orchestration |
| Domain | Business validation (format, rules) |

### 4. Observability via Decorator Pattern (Approach A)

### 5. App Owns Server and Routes
No `server.router()` exposure. App registers routes internally in `create()`.

---

## Identified Gaps

### Gap 1: Router API ✅ RESOLVED
**Status:** [x] Resolved

App owns server internally. Routes registered in factory:
```cpp
// Inside UriShortenerApp::create()
m_server->router().post("/shorten", [this](auto& req, auto& res) {
    handle_shorten(req, res);
});
```

No external exposure of router.

---

### Gap 2: Error-to-HTTP Mapping
**Status:** [ ] To implement

```
DomainError::InvalidShortCode → 400 Bad Request
DomainError::InvalidUrl       → 400 Bad Request
DomainError::LinkNotFound     → 404 Not Found
DomainError::LinkExpired      → 410 Gone
DomainError::LinkAlreadyExists → 409 Conflict
DomainError::CodeGenerationFailed → 500 Internal Server Error
```

---

### Gap 3: JSON Serialization
**Status:** [ ] To implement

- Parse JSON request body → Use Case Input
- Serialize Use Case Output → JSON response

---

### Gap 4: Domain Events
**Status:** [x] Deferred

Using decorators for observability. Events later.

---

### Gap 5: Health Endpoints
**Status:** [ ] To implement

- `GET /health` — Is service running?
- `GET /ready` — Are dependencies reachable?

---

## Existing Abstractions (Good!)

```cpp
// libs/net/router/include/IRequest.h
class IRequest {
    std::string_view path_param(std::string_view key) const;
    std::string_view query_param(std::string_view key) const;
    std::string_view body() const;
    std::string_view method() const;
    std::string_view path() const;
    std::string_view header(std::string_view key) const;
};

// libs/net/router/include/IResponse.h
class IResponse {
    void set_status(int code);
    void set_header(std::string_view key, std::string_view value);
    void write(std::string_view data);
    void close();
};
```

✅ Both HTTP/1.1 and HTTP/2 can implement these.

---

## Implementation Status ✅

### Phase 1: Infrastructure Adapters ✅
- [x] `RandomCodeGenerator` — implements `ICodeGenerator`
- [x] `InMemoryLinkRepository` — implements `ILinkRepository`
- [ ] `RedisLinkRepository` — **DEFERRED** (waiting for sharding/queue infrastructure)

### Phase 2: HTTP Integration ✅
- [x] Handler methods: `handle_shorten`, `handle_resolve`, `handle_delete`, `handle_health`
- [x] Error mapping: `domain_error_to_status()`, `domain_error_to_message()`
- [x] Simple JSON parsing (inline)

### Phase 3: Observability (Decorator) ✅
- [x] `ObservableLinkRepository` wraps `ILinkRepository`
- [x] Timing histograms: `link_repo.*.duration_ms`
- [x] Counters: `link_repo.*.success/error/miss`
- [x] Tracing spans: `LinkRepository.*`
- [x] 5 tests passing

### Phase 4: Composition Root ✅
- [x] `UriShortenerApp` with factory `create()`
- [x] `main.cpp` (6 lines)
- [x] `/health` endpoint

---

## Deferred Items

| Item | Reason |
|------|--------|
| TTL Cleanup | Needs timer infrastructure (timerfd/epoll) or Redis native EXPIRE |
| RedisLinkRepository | Waiting for sharded/shared queue infrastructure |
| Circuit Breaker | Generic resilience component for `libs/core/resilience/` |

---

## Test Commands

```bash
# Build
cmake --preset gcc-debug && cmake --build --preset gcc-debug -j2

# Run all tests
ctest --preset gcc-debug -V

# Run specific tests
ctest --preset gcc-debug -R UriShortenerApp -V
ctest --preset gcc-debug -R Observable -V

# Test HTTP endpoints
./build/gcc-debug/bin/uri_shortener &
sleep 2
curl --http2-prior-knowledge http://localhost:8080/health
curl --http2-prior-knowledge -X POST http://localhost:8080/shorten -d '{"url":"https://example.com"}'
curl --http2-prior-knowledge http://localhost:8080/<short_code>
```

---

## Test Status
- **200 tests passing** (domain + application + observability)
