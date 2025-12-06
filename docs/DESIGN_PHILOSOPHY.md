# Astra Design Philosophy

> **Purpose**: This document defines our **desired state** - how we want our system to look, behave, and evolve.  
> **Status**: Living document (evolves based on production learnings)  
> **Last Updated**: 2025-12-02  
> **Version**: 1.0

---

## Project Philosophy: Learn, Build, Fail (But Not Fragile)

**This is a learning project, not just a product.**

### Core Motivation

- **Learn by building hard things**: Distributed systems, lock-free concurrency, high-performance architectures
- **Build from first principles**: Understand how things work by implementing them, not just using them
- **Fail on complexity, not fragility**: Fail because problems are hard, not because the system is poorly designed
- **Deep mastery over quick shipping**: This project prioritizes understanding over velocity

### Philosophy of Tool Independence

**"Use the best tools, but never depend on them directly"**

- **Abstraction over integration**: Depend on interfaces you control, not vendor APIs
- **Replaceability is non-negotiable**: Swapping infrastructure should not require changing business logic
- **Tools are implementation details**: Application core should not know what infrastructure runs beneath it
- **Dependency inversion always**: High-level components never import low-level infrastructure

**What is coupling**:
- Importing vendor-specific types in business logic
- Calling infrastructure APIs directly from application code
- Hardcoding infrastructure assumptions (file paths, connection strings)

**What is not coupling**:
- Adapter layer that wraps vendor APIs behind your interface
- Configuration that selects which adapter to use
- Tests that inject mock implementations

### Learning Philosophy

**Fail fast, learn deeply** - when something breaks:
- Understand WHY it broke (root cause)
- Learn the underlying principle
- Document the learning
- Update this philosophy

**Production teaches theory** - validate assumptions with real data:
- Design based on principles (SEDA, bulkheads, etc.)
- Validate with production (load testing, profiling)
- Update design based on learnings
- Iterate

---

## Document Purpose

This is Astra's **"desired state manifest"** (like Kubernetes manifests). It describes:
- **WHY** we make architectural decisions
- **WHAT** our target system looks like
- **HOW** we want the system to behave at scale

**ARCHITECTURE.md** describes the **current reality**.  
**DESIGN_GAPS.md** documents the **delta** between this philosophy and current implementation.

---

## Core Philosophy

### The Pragmatic Architect

- **Simplicity \u003e Complexity**: Simple systems are debuggable, extensible, and maintainable
- **Longevity \u003e Speed**: Code spends more time in production than in development
- **Evidence \u003e Opinion**: Decisions driven by production data, not theoretical perfection
- **Product \u003e Project**: We're building for the field, not for completion dates

**Motto**: *"Coding is the last thing we do."* (Design, discuss, validate, then code)

---

## Foundational Principles

### Be Water (Flexibility and Adaptation)

**"Empty your mind, be formless, shapeless — like water."** — Bruce Lee

Water adapts to its container without losing its essence. Our system should be the same.

**Core Tenets:**
- **Vendor Agnostic**: Never marry a technology. OpenTelemetry, not Datadog APIs. Interfaces, not implementations.
- **Graceful Evolution**: Requirements change. Infrastructure shifts. Code should flow with these changes, not break against them.
- **No Dogma**: Use the best tool for the problem. Don't force a pattern because "that's how we do things."
- **Replaceability First**: Build so you can swap any component (database, message queue, observability backend) without rewriting business logic.

**In Practice:**
```cpp
// ❌ RIGID (like ice - brittle and breaks)
#include <prometheus/counter.h>
prometheus::Counter counter = registry.Add(...);

// ✅ FLEXIBLE (like water - adapts to any container)
#include "IMetrics.h"  // Your interface
auto counter = metrics_provider->CreateCounter("requests");
// Today: Prometheus, Tomorrow: OpenTelemetry, Next week: Datadog
```

**Why Water?**
- Water finds the path of least resistance → Code should be easy to change
- Water fills any shape → System adapts to any infrastructure
- Water under pressure becomes stronger → System improves through production stress

### Beginner's Mind (Continuous Learning and Questioning)

**"In the beginner's mind there are many possibilities. In the expert's mind there are few."** — Shunryu Suzuki

Approach every problem like a beginner: curious, open, unassuming.

**Core Tenets:**
- **Question Assumptions**: "We've always done it this way" is not a valid reason.
- **Learn from Failures**: Every production incident is a teacher. Listen to it.
- **Stay Humble**: The most experienced architect can learn from the newest junior developer.
- **Simple First**: Beginners seek simple solutions. Experts overcomplicate. Be the beginner.

**In Practice:**
- Ask "Why?" five times before accepting a design decision
- Assume you're wrong until production proves you right
- When debugging, start with "What did I misunderstand?" not "What's broken?"
- Production data > Expert opinion
- Working code > Elegant architecture

**Mindset:**
```
Beginner: "How does this actually work?"
Expert:   "I know how this works." (stops learning)

Beginner: "Let me test this assumption."
Expert:   "This is obviously correct." (ships bugs)

Beginner: "What's the simplest solution?"
Expert:   "Here's a sophisticated framework..." (over-engineers)
```

**Why Beginner's Mind?**
- Distributed systems are HARD. Hubris leads to outages.
- Today's best practice is tomorrow's anti-pattern. Stay learning.
- Simplicity requires more thinking than complexity.
- You can't learn if you think you already know.

---

### Mandatory Design Philosophies

**ALL design philosophies from these books MUST be adopted and applied**:

#### 1. Clean Architecture (Robert C. Martin)
- **Dependency Rule**: Dependencies point inward; core business logic isolated from frameworks
- **Separation of Concerns**: Clear boundaries between domain, application, and infrastructure
- **Interface Segregation**: Small, focused interfaces
- **Plug-and-Play**: Swap implementations without changing business logic

#### 2. Release It! (Michael Nygard)
- **Bulkheads**: Isolate failures (thread pools, connection pools, circuit breakers)
- **Fail Fast**: Detect errors at boundaries; use `[[nodiscard]]` and validation
- **Circuit Breakers**: Prevent cascading failures in distributed calls
- **Stability Patterns**: Timeouts, retries, backpressure, health checks

#### 3. Effective Modern C++ (Scott Meyers)
- **RAII**: All resources managed via smart pointers and RAII guards
- **Move Semantics**: Eliminate unnecessary copies
- **Type Safety**: Use `auto`, `constexpr`, `noexcept`, `std::string_view`
- **No Raw Pointers**: Ownership must be explicit via smart pointers

#### 4. Designing Data-Intensive Applications (Martin Kleppmann)
- **Scalability**: Horizontal scaling via pods, not vertical via threads
- **Reliability**: Idempotent operations, eventual consistency where appropriate
- **Maintainability**: Data-centric design with clear repository abstractions
- **Observability**: Metrics, tracing, structured logging

---

## Target Architecture

### Performance Goals

| Metric | Target | Rationale |
|:-------|:-------|:----------|
| **Throughput** | 1M+ TPS | Distributed across horizontally scaled pods |
| **Latency (p99)** | \u003c 10ms | End-to-end request processing |
| **Availability** | 99.99% | ~52 minutes downtime/year |
| **Resource Efficiency** | 2 CPU cores/pod | Cost optimization + horizontal scaling |

### Desired Threading Model (SEDA)

**Staged Event-Driven Architecture** with **strict thread role separation**:

```
[HTTP/2 Request] 
  → [Network Threads] (IO/Parsing only)
    → [Worker Threads] (Business Logic + FSM)
      → [IO Service Threads] (DB/Cache/HTTP clients)
      ← [Return to SAME worker thread] (FSM continuity)
    ← [Network Threads] (Response serialization)
  ← [HTTP/2 Response]
```

**Thread Roles**:

| Thread Type | Count (2-core pod) | Responsibility | Queue Type |
|:------------|:-------------------|:---------------|:-----------|
| **Network** | Minimal (nghttp2) | Protocol handling, parsing | Event-driven (Boost.Asio) |
| **Worker** | 1-2 | Business logic, FSM state | **Sharded** (session affinity) |
| **IO Service** | 1 | Blocking operations (DB, HTTP clients) | **Shared** (work stealing) |

**Why this model?**
- **Bulkheads**: Thread pool failures isolated (network ≠ worker ≠ IO)
- **FSM Continuity**: Same worker thread per session → state on one CPU core
- **Cache Locality**: Sharded queues → better L1/L2 cache utilization
- **Backpressure**: Bounded queues prevent OOM under load

---

## Desired Component Architecture

### Layered Design

```
┌─────────────────────────────────────┐
│   Application Layer (Business)      │  ← Domain-specific logic
├─────────────────────────────────────┤
│   HTTP Protocol Layer (HTTP/1.1,2)  │  ← Plug-and-play protocols
├─────────────────────────────────────┤
│   Client Abstraction (DB, Cache)    │  ← Repository pattern
├─────────────────────────────────────┤
│   State Management (FSM)             │  ← HFSM2, Boost.MSM
├─────────────────────────────────────┤
│   Foundation (Concurrency, Logging)  │  ← Core utilities
└─────────────────────────────────────┘
```

**Dependency Flow**: Always **downward** (Clean Architecture dependency rule)

### Interface Design Philosophy

- **Minimal Interfaces**: Only essential methods (GET, POST, PUT, DELETE)
- **Add features when needed**, not speculatively
- **Factory functions over DI frameworks**: Simple \u003e Complex
- **TDD-Driven APIs**: Write tests first to discover the right interface

---

## Desired Operational Characteristics

### Cloud-Native Design

- **Horizontal Scaling**: Scale via Kubernetes HPA (more pods, not bigger pods)
- **Resource Constraints**: 2 CPU cores per application container
  - Total: 4 cores/pod (2 app + 2 Istio sidecar)
- **Stateless Pods**: No local state; all state in Redis/MongoDB
- **Health Checks**: `/health` and `/ready` endpoints for k8s probes

### Resilience Patterns

| Pattern | Purpose | Implementation |
|:--------|:--------|:---------------|
| **Circuit Breaker** | Stop calling failing services | Wrap DB/HTTP clients |
| **Timeout** | Prevent resource exhaustion | All blocking operations |
| **Retry** | Handle transient failures | Exponential backoff |
| **Bulkhead** | Isolate thread pools | WorkerPool, IoWorkerPool |
| **Backpressure** | Reject work under load | Bounded queue capacity |

### Observability

- **Metrics**: Prometheus (latency, throughput, error rate, queue depth)
- **Tracing**: OpenTelemetry (distributed request tracing)
- **Logging**: Structured JSON logs with correlation IDs
- **Dashboards**: Grafana (real-time system health)

---

## Desired Quality Attributes

### Testing Strategy

**Defense in Depth**: Multiple compilers and tools to catch bugs **before production**.

| Phase | Tool | Purpose |
|:------|:-----|:--------|
| **Development** | GoogleTest | Unit tests (TDD) |
| **Pre-commit** | GCC ASan | Memory safety |
| **CI** | Clang TSan | Thread safety |
| **CI** | Valgrind Memcheck | Memory leaks |
| **CI** | Valgrind Helgrind | Data races |
| **Staging** | Load Testing (wrk, k6) | Performance validation |

**Philosophy**: *"Catch bugs in dev, not in the field."*

### Code Quality Standards

- **C++ Core Guidelines**: RAII, modern features, contracts (`[[nodiscard]]`, `noexcept`)
- **No Raw Pointers**: Smart pointers mandatory (`std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`)
- **Production Warnings**: `-Wall -Wextra -Wpedantic` (treat warnings as errors)
- **Mental Dry Run**: Review code 5+ times before compilation
- **Compilation is for verification**, not debugging

---

## Desired Development Workflow

### Design Process

1. **Discussion First**: Design, architecture, implications discussed **before** code
2. **Mental Dry Run**: Walk through logic, edge cases, lifetime issues
3. **TDD**: Write test → implement → refactor
4. **Verification**: Inner loop (single test) → outer loop (all component tests)

### Build Philosophy

- **CMake Presets**: Always use presets (avoid manual flags)
- **Ninja Generator**: Faster builds, better dependency tracking
- **Incremental Builds**: `ninja clean` preserves CMake cache
- **Parallelism**: `-j2` (aligned with 2-core pod design)

### Containerized Development

**ALL development inside container** (build, test, git):
- **Why**: Ensures environment consistency, prevents host/container mismatches
- **Container**: Ubuntu 25.10 with pre-built MongoDB C driver (2.1.2)
- **Tool Versions**: Match production (same GCC/Clang, same Boost 1.88)

---

---

## Configuration Philosophy

### Use Good Tools Through Clean Abstractions

**Philosophy**: Use best-in-class configuration infrastructure, but isolate it behind interfaces.

**Why abstraction layers?**
- **Replaceability**: Swap configuration backends without changing application code
- **Testability**: Inject mock config sources in tests
- **Evolution**: Start simple (files), upgrade to distributed (consensus stores) when needed

**Configuration sources are implementation details**:
- Application reads config through a provider interface
- Provider loads from a source (file watcher, distributed store, API endpoint)
- Source is pluggable - file watching for dev, distributed consensus for production

**Learning progression** (incremental complexity):
- Simple first: File watching with basic synchronization (local development)
- Then concurrency: Lock-free reading with immutable snapshots (performance)
- Then events: Push-based callbacks for components (efficiency)
- Then distributed: Consensus-based backend (multi-instance consistency)

**Each layer teaches a concept**: OS primitives, concurrent data structures, observer pattern, distributed systems

### Configuration Design Principles

**Day 0 / Day 1 / Day 2 Model**:
- **Day 0** (Bootstrap): Cannot start without (server port, thread count, DB URIs)
- **Day 1** (Operational): Hot-reloadable tuning (timeouts, log level, pool sizes)
- **Day 2** (Runtime): API-driven behavior (rate limits, circuit breakers, feature flags)

**Push over Pull**:
- Components **don't poll** config
- Config **pushes** updates via callbacks
- Hot path reads **cached state** (atomic variables)
- Cold path updates cache when config changes

**Abstraction architecture**:
```
Application Layer (Business Logic)
         ↓ depends on
ConfigProvider (Abstraction)
         ↓ depends on
IConfigSource (Interface)
         ↑ implemented by
File Source | Distributed Store Source | API Source
```

**Principle**: Swap source implementation without touching layers above

### Desired Configuration Behavior

**Lock-free reading**:
- Readers never block during config access
- Config snapshots are immutable (copy-on-write pattern)
- Old snapshots remain valid until all readers finish

**Hot reload**:
- Config changes detected automatically (source-dependent mechanism)
- Update flow: detect change → parse → validate → atomic swap
- Components notified via callbacks (push model)
- Invalid config rejected, old config retained (fail safe)

**Failure handling**:
- Parse errors → log warning, keep old config (graceful degradation)
- Missing config at startup → CRASH immediately (fail fast)
- Source unavailable → retry with backoff, use cached config (resilience)

**Validation**:
- Schema validation (types, ranges, required fields)
- Business logic validation (timeouts sensible, values within bounds)
- Fail fast on startup, fail safe on runtime updates
  worker_threads: 2
  io_service_threads: 1

http2_client:
  idle_timeout_sec: 30
  max_connections: 100

resilience:
  circuit_breaker_threshold: 5
  retry_max_attempts: 3
  timeout_ms: 5000
```

---

## What We Deliberately Avoid

| Anti-Pattern | Why Avoid | Alternative |
|:-------------|:----------|:------------|
| **Complex DI Frameworks** | Over-engineering | Simple factory functions |
| **Template Metaprogramming** | Unreadable, slow compile | Use when truly needed |
| **Premature Optimization** | Wastes time | Measure first, optimize second |
| **Monolithic Pods** | Hard to scale | Small pods, scale horizontally |
| **Synchronous Blocking in Workers** | Thread starvation | Async IO on IO service threads |

---

## Evolution Philosophy

### How This Document Evolves

This is a **living document**. It changes when:
1. **Production teaches us**: Empirical data beats theory
2. **Technology advances**: New patterns, libraries, techniques emerge
3. **Requirements shift**: Business needs drive architecture

**When we change philosophy**:
- Document **WHY** (learning from production)
- Update **DESIGN_GAPS.md** (what changed, impact)
- Version the philosophy (DESIGN_PHILOSOPHY_V2.md for major shifts)

### Review Cadence

- **Quarterly**: Review alignment between philosophy, architecture, and gaps
- **Post-Incident**: After production outages, update resilience patterns
- **Annual**: Major revision based on cumulative learnings

---

## Principles in Practice

### Unix Philosophy: Do One Thing Well

- **Each component has ONE responsibility**
- **Composability over monolithic complexity**
- **Small, focused tools** are easier to test and maintain

### TDD: Tests Drive Design

- **Write test before implementation**
- **Design API from user's perspective**
- **Write enough code to pass test, then refactor**

### Fail Fast

- **Validate at boundaries** (request parsing, config loading)
- **Use `[[nodiscard]]`** for critical return values
- **Crash on invariant violations** (don't limp along with corrupt state)

---

## References

### Books (Mandatory Philosophies)

1. **Clean Architecture** - Robert C. Martin
2. **Release It!** - Michael Nygard
3. **Effective Modern C++** - Scott Meyers
4. **Designing Data-Intensive Applications** - Martin Kleppmann

### Related Documents

- [ARCHITECTURE.md](file:///home/siddu/astra/ARCHITECTURE.md) - Current state (how the system IS)
- [DESIGN_GAPS.md](file:///home/siddu/astra/DESIGN_GAPS.md) - Gap analysis (delta between philosophy and reality)
- [INSTRUCTIONS.md](file:///home/siddu/astra/INSTRUCTIONS.md) - AI Agent guidelines for working on this codebase
- [README.md](file:///home/siddu/astra/README.md) - Build instructions and quick start

---

**End of Document**
