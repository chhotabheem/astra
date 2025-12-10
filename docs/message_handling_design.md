# Message-Based Execution Library Design

## Document Purpose

This is a **self-contained implementation guide** for refactoring the Astra execution library from a task-based thread pool to a message-passing architecture with SEDA (Staged Event-Driven Architecture) semantics.

**This document contains everything needed to implement this design without prior context.**

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [Design Principles](#2-design-principles)
3. [Architecture Overview](#3-architecture-overview)
4. [Core Library Design](#4-core-library-design)
5. [Application Layer Design](#5-application-layer-design)
6. [Observability Integration](#6-observability-integration)
7. [TDD Implementation Plan](#7-tdd-implementation-plan)
8. [File Changes Reference](#8-file-changes-reference)
9. [Code Examples](#9-code-examples)
10. [Design Decisions Log](#10-design-decisions-log)

---

## 1. Problem Statement

### Current State

The existing `StripedThreadPool` uses `std::any` for payloads but always casts to `std::function<void()>`:

```cpp
// Current: libs/core/execution/include/Job.h
struct Job {
    JobType type;
    uint64_t session_id;
    std::any payload;
    obs::Context trace_ctx;
};

// Worker always does:
auto task = std::any_cast<std::function<void()>>(job.payload);
task();
```

**Problems:**
1. Type erasure overhead without benefit—always same type
2. No support for heterogeneous message types
3. Pool is coupled to execution semantics (runs tasks)

### Target State

A message-passing pool where:
- Pool **delivers** messages, doesn't interpret them
- Application defines message types and handling
- Type-safe dispatch via `std::variant` + `std::visit`
- Trace context propagates across thread boundaries

---

## 2. Design Principles

| Principle | Application |
|-----------|-------------|
| **Single Responsibility** | Pool delivers. Handler processes. App wires. |
| **Dependency Inversion** | Core depends on abstractions (`IMessageHandler`) |
| **Composition over Inheritance** | App composes handlers, doesn't inherit |
| **Decorator Pattern** | Observability wraps handlers, not inline |
| **Type Safety at Boundaries** | `std::any` in transport, `std::variant` in application |

### Separation of Concerns

| Component | Knows About | Does NOT Know About |
|-----------|-------------|---------------------|
| Router | Path → Handler mapping | Threads, pools, app internals |
| StripedMessagePool | Message delivery, session affinity | Payload contents |
| UriShortenerApp | Composition of all pieces | Router internals |
| UriShortenerMessageHandler | Business logic, payload types | Pool implementation |

---

## 3. Architecture Overview

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            UriShortenerApp                               │
│                          (Composition Root)                              │
│                                                                          │
│  Creates and wires:                                                      │
│  ┌──────────────────┐  ┌────────────────────┐  ┌─────────────────────┐  │
│  │ RequestHandler   │  │ MessageHandler     │  │ StripedMessagePool  │  │
│  │ (IRequestHandler)│  │ (IMessageHandler)  │  │                     │  │
│  └────────┬─────────┘  └─────────┬──────────┘  └──────────┬──────────┘  │
│           │                      │                        │              │
│           │                      └────────────────────────┘              │
│           │                      (pool invokes handler)                  │
│           │                                                              │
│  Decorators wrap for observability:                                      │
│  ┌──────────────────────┐  ┌──────────────────────┐                     │
│  │ ObservableReqHandler │  │ ObservableMsgHandler │                     │
│  └──────────────────────┘  └──────────────────────┘                     │
└───────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Registers request handler
                                    ▼
                          ┌──────────────────┐
                          │     Router       │
                          │ (Path dispatch)  │
                          └──────────────────┘
```

### Message Flow

```
1. HTTP Request arrives at Router
2. Router calls RequestHandler.handle(req, resp)
3. RequestHandler creates HttpRequestMsg, submits to Pool
4. Pool delivers to MessageHandler.handle(msg) on worker thread
5. MessageHandler dispatches via std::visit to onHttpRequest()
6. onHttpRequest() calls DB, creates DbResponseMsg, submits to Pool
7. Pool delivers to MessageHandler.handle(msg)
8. MessageHandler dispatches to onDbResponse()
9. onDbResponse() sends HTTP response
```

---

## 4. Core Library Design

### 4.1 Message Structure

```cpp
// libs/core/execution/include/Message.h

#pragma once
#include <any>
#include <cstdint>
#include <Context.h>

namespace astra::execution {

struct Message {
    uint64_t session_id;      // For session affinity (same session → same worker)
    obs::Context trace_ctx;   // For distributed tracing across threads
    std::any payload;         // Application-defined content
};

} // namespace astra::execution
```

**Design Notes:**
- `session_id`: Hash determines which worker queue receives the message
- `trace_ctx`: Decorator creates child span from this context
- `payload`: Type-erased; application casts to its `std::variant`

### 4.2 Message Handler Interface

```cpp
// libs/core/execution/include/IMessageHandler.h

#pragma once
#include "Message.h"

namespace astra::execution {

class IMessageHandler {
public:
    virtual ~IMessageHandler() = default;
    virtual void handle(Message& msg) = 0;
};

} // namespace astra::execution
```

### 4.3 Striped Message Pool

```cpp
// libs/core/execution/include/StripedMessagePool.h

#pragma once
#include "Message.h"
#include "IMessageHandler.h"
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace astra::execution {

class StripedMessagePool {
public:
    StripedMessagePool(size_t num_threads, IMessageHandler& handler);
    ~StripedMessagePool();

    StripedMessagePool(const StripedMessagePool&) = delete;
    StripedMessagePool& operator=(const StripedMessagePool&) = delete;

    void start();
    void stop();
    bool submit(Message msg);

    size_t thread_count() const { return m_num_threads; }

private:
    struct Worker {
        std::thread thread;
        std::deque<Message> queue;
        std::mutex mutex;
        std::condition_variable cv;
    };

    void worker_loop(size_t index);
    size_t select_worker(uint64_t session_id) const;

    size_t m_num_threads;
    std::vector<std::unique_ptr<Worker>> m_workers;
    IMessageHandler& m_handler;
    std::atomic<bool> m_running{false};
};

} // namespace astra::execution
```

### 4.4 Implementation Notes

**Worker Loop:**
```cpp
void StripedMessagePool::worker_loop(size_t index) {
    Worker& worker = *m_workers[index];
    
    while (m_running) {
        std::optional<Message> msg;
        
        {
            std::unique_lock lock(worker.mutex);
            worker.cv.wait(lock, [&] { 
                return !worker.queue.empty() || !m_running; 
            });
            
            if (!m_running && worker.queue.empty()) break;
            
            if (!worker.queue.empty()) {
                msg = std::move(worker.queue.front());
                worker.queue.pop_front();
            }
        }
        
        if (msg) {
            try {
                m_handler.handle(*msg);
            } catch (const std::exception& e) {
                // Log error, don't crash worker
                // Future: Dead letter queue
            }
        }
    }
}
```

**Session Affinity:**
```cpp
size_t StripedMessagePool::select_worker(uint64_t session_id) const {
    return session_id % m_num_threads;
}
```

---

## 5. Application Layer Design

### 5.1 Payload Types (URI Shortener)

```cpp
// apps/uri_shortener/include/UriMessages.h

#pragma once
#include <string>
#include <variant>
#include <memory>
#include <IRequest.h>
#include <IResponse.h>

namespace uri_shortener {

struct HttpRequestMsg {
    std::shared_ptr<router::IRequest> request;
    std::shared_ptr<router::IResponse> response;
};

struct DbQueryMsg {
    std::string operation;
    std::string data;
    std::shared_ptr<router::IResponse> response;
};

struct DbResponseMsg {
    std::string result;
    bool success;
    std::string error;
    std::shared_ptr<router::IResponse> response;
};

// Type-safe variant
using UriPayload = std::variant<HttpRequestMsg, DbQueryMsg, DbResponseMsg>;

// Helper for std::visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace uri_shortener
```

### 5.2 Request Handler

```cpp
// apps/uri_shortener/include/UriShortenerRequestHandler.h

#pragma once
#include <net/http/IRequestHandler.h>
#include <execution/StripedMessagePool.h>

namespace uri_shortener {

class UriShortenerRequestHandler : public IRequestHandler {
public:
    explicit UriShortenerRequestHandler(astra::execution::StripedMessagePool& pool);

    void handle(HttpRequest& req, HttpResponse& resp) override;

private:
    astra::execution::StripedMessagePool& m_pool;
    
    uint64_t generateSessionId(const HttpRequest& req);
};

} // namespace uri_shortener
```

**Implementation:**
```cpp
void UriShortenerRequestHandler::handle(HttpRequest& req, HttpResponse& resp) {
    uint64_t session_id = generateSessionId(req);
    
    HttpRequestMsg msg{req.path, req.method, req.body, &resp};
    
    astra::execution::Message message{
        session_id,
        obs::Context::current(),
        UriPayload{std::move(msg)}
    };
    
    m_pool.submit(std::move(message));
}
```

### 5.3 Message Handler with std::visit

```cpp
// apps/uri_shortener/include/UriShortenerMessageHandler.h

#pragma once
#include <execution/IMessageHandler.h>
#include <execution/StripedMessagePool.h>
#include "ILinkRepository.h"
#include "UriMessages.h"

namespace uri_shortener {

class UriShortenerMessageHandler : public astra::execution::IMessageHandler {
public:
    UriShortenerMessageHandler(ILinkRepository& repo);
    
    void setPool(astra::execution::StripedMessagePool& pool);
    void handle(astra::execution::Message& msg) override;

private:
    void onHttpRequest(HttpRequestMsg& req, uint64_t session_id, obs::Context& ctx);
    void onDbQuery(DbQueryMsg& query, uint64_t session_id, obs::Context& ctx);
    void onDbResponse(DbResponseMsg& resp);
    
    ILinkRepository& m_repo;
    astra::execution::StripedMessagePool* m_pool{nullptr};
};

} // namespace uri_shortener
```

**Implementation with std::visit:**
```cpp
void UriShortenerMessageHandler::handle(astra::execution::Message& msg) {
    auto& payload = std::any_cast<UriPayload&>(msg.payload);
    
    std::visit(overloaded{
        [this, &msg](HttpRequestMsg& req) {
            onHttpRequest(req, msg.session_id, msg.trace_ctx);
        },
        [this, &msg](DbQueryMsg& query) {
            onDbQuery(query, msg.session_id, msg.trace_ctx);
        },
        [this](DbResponseMsg& resp) {
            onDbResponse(resp);
        }
    }, payload);
}

void UriShortenerMessageHandler::onHttpRequest(HttpRequestMsg& req, 
                                                 uint64_t session_id,
                                                 obs::Context& ctx) {
    // Business logic: parse path, determine action
    auto path = req.request->path();
    if (path == "/shorten") {
        // Create DB query
        DbQueryMsg query{"insert", std::string(req.request->body()), req.response};
        m_pool->submit({session_id, ctx, UriPayload{std::move(query)}});
    }
    // ... other paths
}

void UriShortenerMessageHandler::onDbQuery(DbQueryMsg& query,
                                            uint64_t session_id,
                                            obs::Context& ctx) {
    // Execute DB operation
    auto result = m_repo.shorten(query.data);
    
    DbResponseMsg resp{
        result.has_value() ? result.value() : "",
        result.has_value(),
        result.has_value() ? "" : "Not found",
        query.response
    };
    
    m_pool->submit({session_id, ctx, UriPayload{std::move(resp)}});
}

void UriShortenerMessageHandler::onDbResponse(DbResponseMsg& resp) {
    // Send HTTP response via interface
    resp.response->set_status(resp.success ? 200 : 404);
    resp.response->write(resp.success ? resp.result : resp.error);
    resp.response->close();
}
```

---

## 6. Observability Integration

### 6.1 Decorator Pattern

Observability is added via decorators, NOT inline in business code.

```cpp
// libs/core/execution/include/ObservableMessageHandler.h

#pragma once
#include "IMessageHandler.h"
#include <Span.h>

namespace astra::execution {

class ObservableMessageHandler : public IMessageHandler {
public:
    explicit ObservableMessageHandler(IMessageHandler& inner);
    
    void handle(Message& msg) override;

private:
    IMessageHandler& m_inner;
};

} // namespace astra::execution
```

**Implementation:**
```cpp
void ObservableMessageHandler::handle(Message& msg) {
    // Create child span from message's trace context
    auto span = obs::Span::startWithParent("message.handle", msg.trace_ctx);
    span.setAttribute("session_id", std::to_string(msg.session_id));
    
    try {
        m_inner.handle(msg);
        span.setStatus(obs::Status::OK);
    } catch (const std::exception& e) {
        span.setStatus(obs::Status::ERROR);
        span.setAttribute("error", e.what());
        throw;
    }
}
```

### 6.2 Observable Request Handler

```cpp
// libs/net/http/include/ObservableRequestHandler.h

class ObservableRequestHandler : public IRequestHandler {
    IRequestHandler& m_inner;
public:
    void handle(HttpRequest& req, HttpResponse& resp) override {
        auto span = obs::Span::start("http.request");
        span.setAttribute("path", req.path);
        span.setAttribute("method", req.method);
        
        m_inner.handle(req, resp);
    }
};
```

### 6.3 Trace Context Propagation

**Why `obs::Context` in Message:**

When a message crosses thread boundaries:
1. RequestHandler runs on network thread, captures `obs::Context::current()`
2. Message submitted to pool with context attached
3. Worker thread receives message
4. Decorator creates child span from `msg.trace_ctx`
5. All operations in that handler are children of original request span

**This cannot work with thread-local alone**—must explicitly pass context.

---

## 7. TDD Implementation Plan

### Implementation Sequence

| Order | Component | Dependencies | Description |
|-------|-----------|--------------|-------------|
| **0** | HTTP/2 Layer | None | Prerequisite: shared_ptr, Handler signature change |
| **1** | Core Execution | Order 0 | Message, IMessageHandler, StripedMessagePool |
| **2** | Observable Pool | Order 1 | ObservableMessagePool decorator |
| **3** | App Handlers | Order 1 | UriMessages, RequestHandler, MessageHandler |
| **4** | Handler Decorators | Order 3 | ObservableMessageHandler, ObservableRequestHandler |
| **5** | Composition | Order 2, 3, 4 | Wire everything in UriShortenerApp |
| **6** | Cleanup | Order 5 | Remove deprecated files |

---

### Iteration 0: HTTP/2 Layer Changes (Prerequisite)

**Files to modify:**

| File | Change |
|------|--------|
| `NgHttp2Server.cpp` | Create `shared_ptr<Request/Response>`, upcast to interface |
| `Http2Server.h` | Handler signature to `shared_ptr<IRequest/IResponse>` |
| `Router.h` / `Middleware.h` | Update Handler type |

**Test:**
- Existing HTTP tests should pass (API change is backward compatible if we overload)

---

### Iteration 1: Core Library Foundation

**Test First (write before implementation):**

```cpp
// libs/core/execution/tests/striped_message_pool_test.cpp

TEST(StripedMessagePoolTest, DeliversMessageToHandler) {
    MockHandler handler;
    StripedMessagePool pool(2, handler);
    pool.start();
    
    Message msg{42, obs::Context{}, std::any{}};
    pool.submit(std::move(msg));
    
    // Wait for delivery
    std::this_thread::sleep_for(50ms);
    pool.stop();
    
    EXPECT_EQ(handler.received_count(), 1);
}

TEST(StripedMessagePoolTest, SessionAffinityRoutesToSameWorker) {
    MockHandler handler;
    StripedMessagePool pool(4, handler);
    pool.start();
    
    // Submit 10 messages with same session_id
    for (int i = 0; i < 10; ++i) {
        Message msg{42, obs::Context{}, std::any{}};
        pool.submit(std::move(msg));
    }
    
    std::this_thread::sleep_for(100ms);
    pool.stop();
    
    // All should be on same thread
    auto thread_ids = handler.get_thread_ids();
    EXPECT_EQ(std::set(thread_ids.begin(), thread_ids.end()).size(), 1);
}

TEST(StripedMessagePoolTest, PropagatesTraceContext) {
    MockHandler handler;
    StripedMessagePool pool(2, handler);
    pool.start();
    
    obs::Context ctx = obs::Context::current();
    Message msg{1, ctx, std::any{}};
    pool.submit(std::move(msg));
    
    std::this_thread::sleep_for(50ms);
    pool.stop();
    
    EXPECT_EQ(handler.last_trace_ctx(), ctx);
}
```

**Files to create:**
1. `execution/include/Message.h`
2. `execution/include/IMessageHandler.h`
3. `execution/include/StripedMessagePool.h`
4. `execution/src/StripedMessagePool.cpp`
5. `execution/tests/striped_message_pool_test.cpp`

**Files to delete:**
1. `execution/include/Job.h`
2. `execution/include/StripedThreadPool.h`
3. `execution/src/StripedThreadPool.cpp`

---

### Iteration 2: Observable Pool Decorator

**Test First:**

```cpp
TEST(ObservablePoolTest, EmitsSubmitMetric) {
    MockHandler handler;
    StripedMessagePool core(2, handler);
    ObservableMessagePool pool(core);
    
    pool.start();
    pool.submit({1, obs::Context{}, std::any{}});
    pool.stop();
    
    auto metrics = obs::Metrics::getCounterValue("message_pool.submitted");
    EXPECT_EQ(metrics, 1);
}
```

**Files to create:**
1. `execution/include/ObservableMessagePool.h`
2. `execution/src/ObservableMessagePool.cpp`

---

### Iteration 3: Application Handlers

**Test First:**

```cpp
TEST(UriShortenerMessageHandlerTest, DispatchesHttpRequest) {
    MockRepo repo;
    UriShortenerMessageHandler handler(repo);
    MockPool pool;
    handler.setPool(pool);
    
    HttpRequestMsg req{"/shorten", "POST", "http://example.com", nullptr};
    Message msg{1, obs::Context{}, UriPayload{req}};
    
    handler.handle(msg);
    
    // Should have submitted a DbQueryMsg
    EXPECT_EQ(pool.submitted_count(), 1);
}

TEST(UriShortenerMessageHandlerTest, VisitCoversAllVariants) {
    // Compile-time check: if you add a new variant type,
    // std::visit will fail to compile until you handle it
}
```

**Files to create:**
1. `uri_shortener/include/UriMessages.h`
2. `uri_shortener/include/UriShortenerRequestHandler.h`
3. `uri_shortener/src/UriShortenerRequestHandler.cpp`
4. `uri_shortener/include/UriShortenerMessageHandler.h`
5. `uri_shortener/src/UriShortenerMessageHandler.cpp`

---

### Iteration 4: Observable Handler Decorators

**Test First:**

```cpp
TEST(ObservableMessageHandlerTest, CreatesSpanFromTraceContext) {
    MockHandler inner;
    ObservableMessageHandler handler(inner);
    
    auto parentCtx = obs::Span::start("parent").getContext();
    Message msg{1, parentCtx, std::any{}};
    
    handler.handle(msg);
    
    // Verify child span was created with correct parent
    auto spans = obs::TestBackend::getSpans();
    EXPECT_EQ(spans.back().parentId(), parentCtx.spanId());
}
```

**Files to create:**
1. `execution/include/ObservableMessageHandler.h`
2. `net/http/include/ObservableRequestHandler.h`

---

### Iteration 5: Composition & Integration

**Test First:**

```cpp
TEST(UriShortenerAppIntegration, EndToEndRequest) {
    auto app = UriShortenerApp::create(Config{});
    app->start();
    
    // Simulate HTTP request
    MockHttpRequest req{"/shorten", "POST", "http://example.com"};
    MockHttpResponse resp;
    
    app->getRequestHandler().handle(req, resp);
    
    // Wait for async processing
    std::this_thread::sleep_for(100ms);
    
    EXPECT_EQ(resp.status(), 200);
    EXPECT_FALSE(resp.body().empty());
    
    app->stop();
}
```

**Files to modify:**
1. `uri_shortener/src/UriShortenerApp.cpp`

---

### Iteration 6: Cleanup

**Files to delete/modify:**
1. Delete `execution/include/IThreadPool.h`
2. Modify `execution/include/Executors.h`
3. Update CMakeLists.txt files

---

## 8. File Changes Reference

### Core Library (`libs/core/execution/`)

| File | Action | Notes |
|------|--------|-------|
| `include/Message.h` | NEW | Message struct |
| `include/IMessageHandler.h` | NEW | Handler interface |
| `include/StripedMessagePool.h` | NEW | Pool class declaration |
| `src/StripedMessagePool.cpp` | NEW | Pool implementation |
| `include/ObservableMessageHandler.h` | NEW | Handler decorator |
| `include/Job.h` | DELETE | Replaced by Message |
| `include/IThreadPool.h` | DELETE | Not needed |
| `include/StripedThreadPool.h` | DELETE | Replaced |
| `src/StripedThreadPool.cpp` | DELETE | Replaced |
| `tests/striped_message_pool_test.cpp` | NEW | GTest suite |

### Application (`apps/uri_shortener/`)

| File | Action | Notes |
|------|--------|-------|
| `include/UriMessages.h` | NEW | Payload types |
| `include/UriShortenerRequestHandler.h` | NEW | HTTP handler |
| `src/UriShortenerRequestHandler.cpp` | NEW | Implementation |
| `include/UriShortenerMessageHandler.h` | NEW | Message handler |
| `src/UriShortenerMessageHandler.cpp` | NEW | std::visit dispatch |
| `src/UriShortenerApp.cpp` | MODIFY | Composition root |

---

## 9. Code Examples

### Complete Composition Root

```cpp
// apps/uri_shortener/src/UriShortenerApp.cpp

std::unique_ptr<UriShortenerApp> UriShortenerApp::create(const Config& config) {
    // 1. Create repository with observability
    auto repo = InMemoryLinkRepository::create();
    auto observableRepo = std::make_unique<ObservableLinkRepository>(*repo);
    
    // 2. Create message handler
    auto messageHandler = std::make_unique<UriShortenerMessageHandler>(*observableRepo);
    auto observableMessageHandler = std::make_unique<ObservableMessageHandler>(*messageHandler);
    
    // 3. Create pool with decorated handler
    auto pool = std::make_unique<StripedMessagePool>(
        config.worker_threads, 
        *observableMessageHandler
    );
    
    // 4. Wire pool back to message handler (for submitting follow-up messages)
    messageHandler->setPool(*pool);
    
    // 5. Create request handler with pool
    auto requestHandler = std::make_unique<UriShortenerRequestHandler>(*pool);
    auto observableRequestHandler = std::make_unique<ObservableRequestHandler>(*requestHandler);
    
    // 6. Assemble app
    return std::unique_ptr<UriShortenerApp>(new UriShortenerApp(
        std::move(repo),
        std::move(observableRepo),
        std::move(messageHandler),
        std::move(observableMessageHandler),
        std::move(pool),
        std::move(requestHandler),
        std::move(observableRequestHandler)
    ));
}

IRequestHandler& UriShortenerApp::getRequestHandler() {
    return *m_observableRequestHandler;
}

void UriShortenerApp::start() {
    m_pool->start();
}

void UriShortenerApp::stop() {
    m_pool->stop();
}
```

---

## 10. Design Decisions Log

### Decision 1: Why `std::any` at Transport Layer?

**Options considered:**
1. `std::function<void()>` - Simple but no heterogeneous messages
2. `std::any` - Flexible, application defines types
3. Template-based - Zero overhead but complex

**Decision:** `std::any` because applications have different message types (HTTP, DB, Cache).

---

### Decision 2: Why `std::variant` in Application?

**Options considered:**
1. Runtime type checking with `std::any_cast` everywhere
2. `std::variant` with compile-time exhaustive `std::visit`

**Decision:** `std::variant` for compile-time type safety. If you add a new message type and forget to handle it, compiler errors.

---

### Decision 3: Where Does `obs::Context` Live?

**Options considered:**
1. In `Message` struct (explicit)
2. Generic metadata map (hides dependency)
3. Carrier interface (purist)
4. Template parameter (complex)

**Decision:** Option 1 (in Message). Both `execution` and `observability` are core libraries. The dependency is core→core, which is acceptable. Trace context is transport-level metadata like `session_id`.

---

### Decision 4: Why Decorator Pattern for Observability?

**Options considered:**
1. Inline `span.start()` in every handler
2. Decorator wraps handlers

**Decision:** Decorator. Keeps business logic clean. Observability code in one place. Same pattern already used for `ObservableLinkRepository`.

---

### Decision 5: Why App is Composition Root Only?

**Options considered:**
1. App implements `IRequestHandler`
2. App composes handlers, exposes via getter

**Decision:** Option 2. App should wire components, not be a component. Single Responsibility Principle.

---

### Decision 6: Router Coupling to Pool?

**Rejected:** Router should NOT know about pool.

**Reason:** Router is pure dispatch (path → handler). Threading is application's internal concern. If Router knew about pool, you couldn't use synchronous handlers for some paths.

---

## 11. HTTP/2 Layer Lifetime Management

### The Problem

For async processing, handlers must return immediately while worker threads process later. Current stack-local Request/Response are destroyed when handler returns.

### Core Principle

**nghttp2 owns connections. Application gets interface smart pointers. Internal weak_ptr ensures thread-safe access.**

### The Solution

#### 1. NgHttp2Server creates shared_ptr (concrete types)

```cpp
// NgHttp2Server.cpp - on_data callback
struct Context {
    std::shared_ptr<ResponseHandle> response_handle;
    std::shared_ptr<Request> request;   // Concrete http2server::Request
    std::shared_ptr<Response> response; // Concrete http2server::Response
    Server::Handler handler;
};

// Create on heap
ctx->request = std::make_shared<Request>(/* ... */);
ctx->response = std::make_shared<Response>(/* ... */);
ctx->response->m_impl->response_handle = ctx->response_handle;  // weak_ptr to handle

// Pass as interface smart pointers to handler
ctx->handler(
    std::static_pointer_cast<IRequest>(ctx->request),
    std::static_pointer_cast<IResponse>(ctx->response)
);
```

#### 2. Handler receives shared_ptr to interfaces

```cpp
// Handler signature (new)
using Handler = std::function<void(std::shared_ptr<IRequest>, std::shared_ptr<IResponse>)>;

void UriShortenerRequestHandler::handle(std::shared_ptr<IRequest> req, 
                                         std::shared_ptr<IResponse> res) {
    // Submit to pool - shared_ptr keeps objects alive
    pool.submit({session_id, trace_ctx, RequestPayload{req, res}});
    // Handler returns immediately - objects survive via shared_ptr in message
}
```

#### 3. Application payload uses interface smart pointers

```cpp
// Application layer - only sees interfaces
struct RequestPayload {
    std::shared_ptr<IRequest> request;
    std::shared_ptr<IResponse> response;
};
```

#### 4. Response internally uses weak_ptr for thread-safe access

```cpp
// Response::Impl (existing pattern)
class Response::Impl {
    std::weak_ptr<ResponseHandle> response_handle;  // Not shared!
};

void Response::close() {
    if (auto handle = m_impl->response_handle.lock()) {
        handle->send(m_body);  // Stream alive, send data
    } else {
        // Stream closed, gracefully dropped
    }
}
```

### Ownership Model

```
NgHttp2Server (owner)
├── shared_ptr<ResponseHandle> ──→ Contains send function + alive flag
├── shared_ptr<Response> ────────→ Contains body buffer
│   └── Impl.response_handle ────→ weak_ptr<ResponseHandle>
└── shared_ptr<Request> ─────────→ Contains path, method, body

Handler receives: shared_ptr<IRequest>, shared_ptr<IResponse>

Application payload holds: shared_ptr<IRequest>, shared_ptr<IResponse>

Worker uses: payload.response->close() 
             └── Internally: weak_ptr.lock() → safe regardless of stream state
```

### Why This Works

| Scenario | Behavior |
|----------|----------|
| Worker finishes before stream closes | `weak_ptr.lock()` succeeds, data sent |
| Stream closes while worker processing | Request/Response objects survive (held by payload), but `weak_ptr.lock()` fails on ResponseHandle → graceful drop |
| Application holds dangling pointer | Impossible - uses shared_ptr<IRequest/IResponse> |

### What Application Never Sees

- `http2server::Request` / `http2server::Response` (concrete types)
- `ResponseHandle` (internal detail)
- `weak_ptr` (hidden inside Response implementation)

### File Changes for HTTP/2 Layer

| File | Change |
|------|--------|
| `NgHttp2Server.cpp` | Create shared_ptr, upcast to interface |
| `Http2Server.h` | Change Handler signature to shared_ptr |
| `Router.h/cpp` | Update Handler type |

---

## Future Considerations

1. **Resiliency (Circuit Breaker):** Add via decorator wrapping pool
2. **Pool Metrics:** Add `ObservableMessagePool` decorator for queue depth, latency
3. **Dead Letter Queue:** Failed messages after retries
4. **Back-pressure:** Block or drop when queue full
