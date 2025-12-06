# URI Shortener Domain-Driven Design Integration

## The Core Principle

```
┌─────────────────────────────────────────────────────────────────┐
│                         main.cpp                                │
│                    (Composition Root)                           │
│   Creates & wires everything. Knows ALL concrete types.         │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌───────────────┐    ┌───────────────┐    ┌───────────────┐
│   HTTP Layer  │    │Infrastructure │    │ Observability │
│   (Adapters)  │    │   Adapters    │    │   Adapters    │
│               │    │               │    │               │
│ HttpController│    │ RedisRepo     │    │ OTelBackend   │
│ maps HTTP     │    │ MongoRepo     │    │               │
│ to use cases  │    │ RandomCodeGen │    │               │
└───────────────┘    └───────────────┘    └───────────────┘
        │                     │                     │
        │           implements                      │
        │                     ▼                     │
        │            ┌───────────────┐              │
        │            │ Domain Ports  │              │
        └──────────► │ ILinkRepository│ ◄───────────┘
                     │ ICodeGenerator │
                     └───────────────┘
                              ▲
                              │ depends on (interface only)
                     ┌───────────────┐
                     │  Use Cases    │
                     │  ShortenLink  │
                     │  ResolveLink  │
                     └───────────────┘
```

---

## 1. HTTP Router/Server (v1.1 & v2)

**Current libs:** `libs/net/http/v1/server`, `libs/net/http/v2/server`

**How to integrate:**

```cpp
// Infrastructure layer - NOT in domain!
// apps/uri_shortener/infrastructure/http/HttpController.h

class UriShortenerController {
public:
    UriShortenerController(
        std::shared_ptr<ShortenLink> shorten_use_case,
        std::shared_ptr<ResolveLink> resolve_use_case
    );

    // Maps HTTP → Use Case → HTTP
    HttpResponse handle_shorten(const HttpRequest& req);
    HttpResponse handle_resolve(const HttpRequest& req);
};
```

**Changes needed in existing libs?**

| Library | Change Needed? | Why |
|---------|---------------|-----|
| `http/v1/server` | **No** | Already provides `HttpRequest`/`HttpResponse` abstraction |
| `http/v2/server` | **No** | Same abstraction |
| Router | **Maybe** | Need to check if router can dispatch to methods |

**Key point:** The controller lives in `apps/uri_shortener/infrastructure/http/`, NOT in domain or application.

---

## 2. Database Clients (Redis, MongoDB)

**Current libs:** `libs/data/redis`, `libs/data/mongo`

**How to integrate:**

```cpp
// apps/uri_shortener/infrastructure/persistence/RedisLinkRepository.h

class RedisLinkRepository : public ILinkRepository {
public:
    explicit RedisLinkRepository(std::shared_ptr<RedisClient> client);

    Result<void, DomainError> save(const ShortLink& link) override;
    Result<ShortLink, DomainError> find_by_code(const ShortCode& code) override;
    // etc.

private:
    std::shared_ptr<RedisClient> m_client;

    // Serialize/deserialize logic lives HERE, not in domain
    std::string serialize(const ShortLink& link);
    ShortLink deserialize(const std::string& data);
};
```

**Changes needed in existing libs?**

| Library | Change Needed? | Why |
|---------|---------------|-----|
| `libs/data/redis` | **Maybe** | Check if client returns `Result<T, E>` or throws |
| `libs/data/mongo` | **Maybe** | Same check |

**Question:** Do current Redis/Mongo clients use exceptions or `Result` for error handling?

---

## 3. Config

**Current lib:** `libs/core/config`

**How to integrate:**

```cpp
// In main.cpp (Composition Root)
auto config = ConfigProvider::create(/* sources */);

auto expiration_default = config->get<std::chrono::seconds>("uri_shortener.default_expiration");
auto code_length = config->get<int>("uri_shortener.code_length");

// Pass config values to constructors, NOT the config object itself
auto code_generator = std::make_shared<RandomCodeGenerator>(code_length);
```

**Key insight:** Domain doesn't know about "config". It receives **values** via constructor injection.

**Changes needed?**

| Library | Change Needed? | Why |
|---------|---------------|-----|
| `libs/core/config` | **No** | Config is only used in Composition Root |

---

## 4. Observability (Tracing, Metrics, Logging)

This is the most nuanced. There are **3 approaches**:

### Approach A: Observability in Infrastructure Adapters (Recommended)

```cpp
class ObservableRedisRepository : public ILinkRepository {
public:
    ObservableRedisRepository(
        std::shared_ptr<ILinkRepository> inner,  // Decorator pattern
        std::shared_ptr<Metrics> metrics
    );

    Result<void, DomainError> save(const ShortLink& link) override {
        auto timer = metrics->start_timer("repo.save");
        auto result = m_inner->save(link);
        if (result.is_err()) {
            metrics->increment("repo.save.error");
        }
        return result;
    }
};
```

### Approach B: Domain Events + Event Handler

```cpp
// Domain emits events (no logging knowledge)
class ShortLink {
    static CreateResult create(...) {
        // ... create link ...
        DomainEventPublisher::publish(LinkCreated{code, url});
        return ...;
    }
};

// Infrastructure subscribes
class ObservabilityEventHandler {
    void on(const LinkCreated& event) {
        m_logger->info("Link created: {}", event.code);
        m_metrics->increment("links.created");
    }
};
```

### Approach C: Cross-cutting via AOP/Interceptors

This is more complex and not typical in C++.

**Recommendation:** **Approach A** (Decorator pattern) for now.

**Changes needed?**

| Library | Change Needed? | Why |
|---------|---------------|-----|
| `libs/core/observability` | **No** | Used in decorators, not domain |

---

## Summary: What Changes May Be Needed

| Library | Changes |
|---------|---------|
| HTTP libs | Probably none. Just use as-is. |
| Redis/Mongo | Check error handling (exceptions vs Result) |
| Config | None. Only used in main.cpp. |
| Observability | None. Decorator pattern at infrastructure layer. |

---

## Open Questions

1. **Redis/Mongo error handling** — Do current clients throw exceptions or return error codes?
2. **Router flexibility** — Can the router dispatch to controller methods, or does it only support free functions?
3. **Observability approach** — Do you prefer Decorator pattern or Domain Events?

---

## Implementation Status

### Completed (58 tests passing)
- Domain Layer: ShortCode, OriginalUrl, ExpirationPolicy, ShortLink
- Domain Ports: ILinkRepository, ICodeGenerator
- Application Use Cases: ShortenLink, ResolveLink, DeleteLink

### Remaining
- Infrastructure adapters (Redis/Mongo implementations of ILinkRepository)
- HTTP Controller (maps HTTP to use cases)
- Composition Root (main.cpp)
