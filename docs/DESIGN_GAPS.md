# Astra Design Gaps

> **Purpose**: Documents the **delta** between our desired state ([DESIGN_PHILOSOPHY.md](file:///home/siddu/astra/DESIGN_PHILOSOPHY.md)) and current reality ([ARCHITECTURE.md](file:///home/siddu/astra/ARCHITECTURE.md)).  
> **Status**: Living document (updated as gaps close or new gaps emerge)  
> **Last Updated**: 2025-12-02  
> **Version**: 1.0

---

## How to Read This Document

Each gap entry includes:
- **Gap**: What's missing or incomplete
- **Philosophy Reference**: Which desired state principle is not yet met
- **Current State**: What we have today
- **Priority**: High / Medium / Low
- **Reason**: Why we're accepting this gap (pragmatic staging, complexity, dependencies)
- **Target**: When we plan to close the gap
- **Risk Mitigation**: How we're managing the risk of this gap

---

## Critical Gaps (High Priority)

### 1. No Circuit Breaker Implementation

**Philosophy Reference**: Release It! - Prevent cascading failures

**Current State**: 
- HTTP clients, DB clients, and cache clients have **no circuit breaker**
- Failures propagate indefinitely (retry storms possible)

**Priority**: **High**

**Reason**: Core request/response flow must stabilize first before adding resilience patterns

**Target**: Q2 2026 (after URI shortener production deployment)

**Risk Mitigation**:
- Aggressive timeouts on all blocking operations
- Manual fallback procedures documented
- Prometheus alerts on error rates

---

### 2. No Configuration Module

**Philosophy Reference**: DESIGN_PHILOSOPHY.md - Configuration Strategy (future YAML/JSON)

**Current State**:
- All parameters **hardcoded** (timeouts, thread counts, connection pools)
- No environment-based configs (dev/staging/prod)
- No hot reload capability

**Priority**: **High**

**Reason**: Avoid premature configuration complexity; interfaces must stabilize first

**Target**: Q1 2026 (after core interfaces finalized)

**Risk Mitigation**:
- Hardcoded values are conservative (safe defaults)
- Recompile/redeploy required for tuning
- Document all hardcoded values in code comments

---

### 3. No Distributed Tracing

**Philosophy Reference**: DESIGN_PHILOSOPHY.md - Observability (OpenTelemetry)

**Current State**:
- No OpenTelemetry integration
- No distributed request tracing across services
- Debugging multi-service flows is manual (log correlation)

**Priority**: **High**

**Reason**: Focus on core functionality; observability comes after basic flow works

**Target**: Q3 2026 (after microservice mesh grows beyond 3 services)

**Risk Mitigation**:
- Structured logging with correlation IDs (manual tracing)
- Prometheus metrics capture request flow health
- Staging environment for testing before production

---

## Important Gaps (Medium Priority)

### 4. HTTP/2 Client Not Fully Production-Ready

**Philosophy Reference**: DESIGN_PHILOSOPHY.md - HTTP Client Design (connection multiplexing)

**Current State**:
- HTTP/2 client **exists** but not fully battle-tested
- Connection pooling works, but edge cases untested (connection loss, server push)
- No retry logic on transient failures

**Priority**: **Medium**

**Reason**: URI shortener doesn't require HTTP/2 client yet (only server)

**Target**: Q2 2026 (when we need service-to-service calls)

**Risk Mitigation**:
- Comprehensive unit tests exist
- Fallback to HTTP/1.1 if HTTP/2 client fails
- Manual testing in staging before production use

---

### 5. Main Server Integration Not Complete

**Philosophy Reference**: DESIGN_PHILOSOPHY.md - Application Layer

**Current State**:
- `server/` directory exists but **not integrated** into build
- No main application entry point
- URI shortener is standalone, not part of main server

**Priority**: **Medium**

**Reason**: URI shortener validates architecture; full server integration comes after

**Target**: Q4 2025 (after URI shortener production validation)

**Risk Mitigation**:
- URI shortener is reference implementation
- All components tested independently
- Integration path is clear (just wiring, no new design)

---

### 6. Prometheus Metrics Not Fully Instrumented

**Philosophy Reference**: DESIGN_PHILOSOPHY.md - Observability (Prometheus metrics)

**Current State**:
- Prometheus client library **exists**
- Metrics **not instrumented** in all components (router, worker pools, DB clients)
- No Grafana dashboards

**Priority**: **Medium**

**Reason**: Core flow must work before adding observability overhead

**Target**: Q1 2026 (after production deployment)

**Risk Mitigation**:
- Can add metrics incrementally without code restructure
- Structured logging provides basic observability
- CTest ensures component correctness without metrics

---

## Lower Priority Gaps (Nice to Have)

### 7. Kubernetes Deployment Manifests Missing

**Philosophy Reference**: DESIGN_PHILOSOPHY.md - Cloud-Native Design

**Current State**:
- No Kubernetes YAML manifests (Deployment, Service, HPA, ConfigMap)
- No Helm charts
- Deployment is manual (Docker only)

**Priority**: **Low**

**Reason**: Development environment is Docker-first; k8s deployment comes closer to production

**Target**: Q3 2026 (before first production deployment)

**Risk Mitigation**:
- Docker environment matches production closely
- k8s manifest creation is straightforward (not a design risk)
- Can deploy to k8s manually before automation

---

### 8. Load Testing Suite Not Implemented

**Philosophy Reference**: DESIGN_PHILOSOPHY.md - Testing Strategy (wrk, k6 load testing)

**Current State**:
- No load testing framework (wrk, k6, or custom)
- 1M+ TPS target **not validated**
- No performance benchmarking suite

**Priority**: **Low**

**Reason**: Functional correctness first, performance validation second

**Target**: Q2 2026 (after URI shortener deployed)

**Risk Mitigation**:
- Development testing with curl validates basic performance
- GCC/Clang sanitizers catch memory/thread safety issues
- Can run ad-hoc wrk tests before production

---

### 9. State Machine Integration Incomplete

**Philosophy Reference**: DESIGN_PHILOSOPHY.md - State Management Layer (HFSM2, Boost.MSM)

**Current State**:
- HFSM2 and Boost.MSM wrappers **exist**
- **Not integrated** into URI shortener or any real workflow
- FSM continuity pattern not validated in production

**Priority**: **Low**

**Reason**: URI shortener is simple (no complex state machines needed yet)

**Target**: Q4 2026 (when we build stateful workflows like payment processing)

**Risk Mitigation**:
- FSM libraries are tested independently
- Thread affinity (sharded queues) ensures same-worker routing
- Can add FSM to existing code without major refactor

---

### 10. No Retry Logic with Exponential Backoff

**Philosophy Reference**: Release It! - Retry Pattern

**Current State**:
- No retry logic in HTTP clients or DB clients
- Transient failures cause immediate user-facing errors

**Priority**: **Low**

**Reason**: Focus on correctness first; retry logic adds complexity

**Target**: Q2 2026 (after circuit breaker implementation)

**Risk Mitigation**:
- Aggressive timeouts prevent indefinite hangs
- Error responses are clear (caller can retry manually)
- Staging environment catches most transient failures

---

## Design Decisions (Not Gaps)

These are **intentional choices**, not gaps to close:

### 1. No C++20 Coroutines

**Decision**: Use C++17; defer coroutines until team has time to learn them

**Rationale**:
- C++17 is well-understood; no learning curve
- Boost.Asio callbacks work fine for async IO
- Coroutines add complexity without clear benefit (for now)

**Future**: May revisit when async chains get complex

---

### 2. Hardcoded Configuration (Current State)

**Decision**: Hardcode timeouts, thread counts, connection pools initially

**Rationale**:
- Avoid premature configuration complexity
- Core interfaces must stabilize first
- Easier to refactor when patterns emerge

**Future**: Configuration module (YAML/JSON) in Q1 2026

---

### 3. No Dependency Injection Framework

**Decision**: Use simple factory functions, not complex DI frameworks

**Rationale**:
- DI frameworks are over-engineered for our use case
- Factory functions are explicit, testable, and simple
- Aligns with "No Ivory Tower" philosophy

**Future**: This is a permanent decision (not a gap)

---

## Gap Trends

### Closing Gaps

| Gap | Status | Completion |
|:----|:-------|:-----------|
| Router with middleware | ✅ Closed | 2025-11-25 |
| HTTP/2 server implementation | ✅ Closed | 2025-11-24 |
| MongoDB client abstraction | ✅ Closed | 2025-11-22 |
| Valgrind integration | ✅ Closed | 2025-11-26 |

### Growing Gaps (New Needs)

| Gap | Reason | Target |
|:----|:-------|:-------|
| Multi-region database replication | Production scale requirement | TBD |
| API versioning strategy | Production requirement | Q2 2026 |

---

## Prioritization Framework

We prioritize gaps based on:

1. **Production Impact**: Does this gap risk user-facing failures?
2. **Development Velocity**: Does this gap slow down feature development?
3. **Technical Debt**: Does this gap make future changes harder?
4. **Dependencies**: Can we close other gaps first?

**High Priority**: Production impact or blocks other work  
**Medium Priority**: Improves development velocity or reduces tech debt  
**Low Priority**: Nice to have, not blocking

---

## Review Process

### When We Update This Document

- **After each major feature**: Update gaps closed or newly discovered
- **Quarterly**: Review all gaps and reprioritize
- **Post-incident**: Add gaps revealed by production failures
- **Philosophy changes**: Add gaps when desired state changes

### Ownership

- **DESIGN_PHILOSOPHY.md**: Tech lead (strategic vision)
- **ARCHITECTURE.md**: Team (current implementation)
- **DESIGN_GAPS.md**: Architect (gap analysis and prioritization)

---

## References

### Related Documents

- [DESIGN_PHILOSOPHY.md](file:///home/siddu/astra/DESIGN_PHILOSOPHY.md) - Desired state (how we WANT the system)
- [ARCHITECTURE.md](file:///home/siddu/astra/ARCHITECTURE.md) - Current state (how the system IS)
- [README.md](file:///home/siddu/astra/README.md) - Build instructions and quick start

---

**End of Document**

---

# Architectural Review: The Good, The Bad, and The Ugly

**Date:** 2025-12-03
**Reviewer:** Antigravity (Agentic AI)
**Perspective:** Senior Pragmatic Distributed System Architect

## Executive Summary
The Astra project demonstrates a strong foundation in modern C++ practices and build tooling. The use of RAII, PIMPL, and extensive testing infrastructure is commendable. However, there are significant architectural risks related to synchronous blocking I/O in potentially asynchronous paths, inconsistent threading models, and leaky abstractions that couple business logic to specific driver implementations.

---

## The Good (Strengths)

### 1. Modern C++ & Code Quality
- **RAII & Smart Pointers**: The codebase consistently uses `std::unique_ptr` and `std::shared_ptr` for resource management, minimizing memory leak risks.
- **PIMPL Idiom**: Used effectively in `JsonDocument`, `Http2Server`, and `ZookeeperClient` to decouple interfaces from implementation details and speed up compilation.
- **Strong Typing**: Usage of `std::optional` and `enum class` improves safety and expressiveness.

### 2. Concurrency Design
- **Sharded Worker Pool**: The `concurrency::WorkerPool` implementation uses a sharded queue design (one queue per thread). This is an excellent pattern for high-performance systems as it eliminates contention on a central task queue and improves cache locality.
- **Executor Abstraction**: The `IExecutor` interface allows for easy swapping of threading models (e.g., inline for testing vs. thread pool for production).

### 3. Build & Test Infrastructure
- **CMake Presets**: The project leverages CMake presets (`gcc-debug`, `clang-asan`, etc.), standardizing the build process across environments.
- **Sanitizers & Valgrind**: First-class support for AddressSanitizer, ThreadSanitizer, and Valgrind indicates a high commitment to correctness and stability.
- **Dependency Injection**: Core services like `UriService` and `ConfigProvider` are designed with dependency injection, making them highly testable.

---

## The Bad (Weaknesses & Debt)

### 1. Leaky Abstractions
- **Http1Server**: The header `Http1Server.h` exposes `boost::asio` types (`io_context`, `tcp::socket`). This leaks implementation details to consumers and forces a dependency on Boost.Asio for anyone including the header.
- **MongoClient**: The `MongoClient` interface exposes `bsoncxx::document::view`. This tightly couples the application's domain logic to the MongoDB driver's specific types, making it hard to swap storage engines or mock data without pulling in the driver.

### 2. Inconsistent Threading Models
- **Fragmented Thread Management**: While there is a dedicated `concurrency` module, `Http1Server` manages its own `std::vector<std::thread>`. This leads to "thread explosion" and lack of centralized resource control. All components should ideally submit work to the central `WorkerPool` or `IExecutor`.

### 3. Complexity Overhead
- **Boost.MSM**: Using `Boost.MSM` for state machines introduces significant compile-time overhead and cognitive load. Unless the state machine logic is extremely complex, lighter alternatives (like `hfsm2` or simple `std::variant` based state machines) are often more pragmatic.

---

## The Ugly (Risks & Anti-Patterns)

### 1. Synchronous I/O in Async Paths (Critical Risk)
- **Blocking Clients**: Both `ZookeeperClient` and `MongoClient` expose purely synchronous, blocking APIs (`create`, `get`, `findOne`).
- **The Danger**: If these synchronous methods are called from within the `Http1Server` or `Http2Server` request handlers (which run on worker threads), they will **block the worker thread** waiting for network I/O. In a high-throughput system, this will quickly exhaust the thread pool and cause a complete service stall (latency spikes and timeouts), defeating the purpose of an async architecture.

### 2. Singleton Abuse
- **PrometheusManager**: Implemented as a Singleton. Singletons hide dependencies, make unit testing difficult (shared state between tests), and introduce global state that can lead to initialization order issues.

### 3. Performance Bottlenecks in Observability
- **String Lookups**: `PrometheusManager` uses `std::map<std::string, ...>` to look up metric families. If metrics are recorded in the hot path (e.g., per-request), these string lookups and the associated mutex contention will become a significant performance penalty. Metrics handles should be cached by the caller or static.

---

## Recommendations

1.  **Make I/O Async**: Refactor `MongoClient` and `ZookeeperClient` to return `std::future` or take callbacks, or ensure they are wrapped in tasks submitted to a dedicated "blocking I/O" thread pool, separate from the CPU-bound worker pool.
2.  **Unify Threading**: Refactor `Http1Server` to accept an `IExecutor` or `IWorkerPool` instead of spawning its own threads.
3.  **Seal Abstractions**: Wrap `bsoncxx` types in domain-specific DTOs or a generic `Document` abstraction (like the new `JsonDocument`) to decouple from the driver.
4.  **Kill Singletons**: Pass `PrometheusManager` (or an `IMetrics` interface) as a dependency to components that need it.
