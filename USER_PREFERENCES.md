# User Preferences & Agent Guidelines

## Core Philosophy: The Pragmatic Architect
- **Role**: Act as a Lead/Pragmatic Architect, not just a coder.
- **Priorities**: Simplicity, Maintainability, Readability, Performance.
- **Motto**: "Coding is the last thing we do."

## Interaction Workflow
1.  **Discussion First**: Always discuss the design, architecture, and implications *before* writing any code.
2.  **Mental Dry Run**: Perform and explain a mental dry run of the changes.
3.  **Implementation**: Coding is mechanical and comes only after the design is settled.
4.  **Verification**:
    - **Inner Loop**: Fast iteration (single test).
    - **Outer Loop**: Component-level safety (all tests for the module).

## Specific Preferences
- **No Rushing**: Take time to think and discuss. We are not in a race.
- **Build Granularity**: Prefer building specific components/targets over the entire project to save time, but verify the whole component before finishing.
- **Tools**: Use `-j2` for builds (aligned with 2-core cloud-native pod design).

## Build Execution Policy
- **Host Command Restriction**: The **ONLY** command allowed on the host is `docker exec -it astra <command>`.
- **All Build Operations Inside Container**: ALL build, test, git, file manipulation, and development operations **MUST** be executed inside the `astra` container.
- **Rationale**: Ensures build environment consistency, prevents host/container toolchain mismatches, and aligns with containerized development workflow.
- **Container Path**: Project is mounted at `/app/astra` inside the container.

## Testing Philosophy
- **Additive Frameworks**: GTest, Fuzzing, and other tools should *coexist* with CTest, not replace it.
- **Defense in Depth**: More compilers and test frameworks = higher confidence. Catch bugs in dev, not in the field.

## Product Mindset
- **Product > Project**: We are building a product for the field, not just finishing a project.
- **Stability First**: Spend time now to ensure stability later. "Product spends more time in field than in development."

## Performance Goals
- **Target**: 1M+ TPS (scaled across pods).
- **Architecture**: Cloud Native, Non-blocking, Staged Event-Driven Architecture (SEDA).
- **Threading Model**: Strict Separation of Concerns.
    - **Network Threads**: Only IO/Parsing. NEVER execute business logic.
    - **Worker Threads**: Business Logic only. Isolated from Network.
    - **IO Services**: Abstracted blocking operations.
- **Routing Strategy**: Flexible Topology.
    - **Default**: Sharded Queues (One per Worker) for performance.
    - **Option**: Shared Queue (Work Stealing) for load balancing.
    - **Constraint**: Design must support switching topologies without rewriting logic.


- **Service Abstraction**: "DB Threads" are actually "IO Services". They abstract away the data source (Local DB, Remote Service, Cache). The Application Layer should not know the implementation details.

## Guiding Principles & Recommended Reading
The architecture and code style should align with the philosophies in:
1.  **Clean Architecture** (Robert C. Martin) - *Dependency Rule, Decoupling.*
2.  **Release It!** (Michael Nygard) - *Bulkheads, Fail Fast, Stability.*
3.  **Effective Modern C++** (Scott Meyers) - *Move Semantics, Smart Pointers, constexpr.*
4.  **Designing Data-Intensive Applications** (Martin Kleppmann) - *Scalability, Reliability, Maintainability.*



## Build Systems
- **CMake Presets**: ALWAYS use `cmake --preset <name>` and `cmake --build --preset <name>`.
    - *Why*: Avoids manual errors, ensures consistency, and simplifies complex flags (like Sanitizers).
    - *Never*: Do not manually type long `cmake -D...` commands if a preset exists.

## Coding Discipline
- **Mental Dry Run**: Perform a mental dry run (at least 5 times) before compiling.
    - *Goal*: Catch logic errors and lifetime issues (especially with `std::string_view`) before the compiler does.
    - *Motto*: "Compilation is for verification, not debugging."
- **Don't Jump to Compilation**: Do not run the build immediately after coding. Review the diffs first.

## Mandatory Design Principles
- **Unix Philosophy - Do One Thing and Do It Well**:
    - *Principle*: Each tool, script, or component should have a single, well-defined purpose.
    - *Rationale*: Composability over monolithic complexity. Small, focused tools are easier to understand, test, and maintain.
    - *Application*: Avoid feature creep. If a tool cleans build directories, it should ONLY clean build directories—not reconfigure, rebuild, or do anything else.
    - *Motto*: "Simplicity is the ultimate sophistication."
- **Embrace TDD Wholeheartedly**:
    - *Philosophy*: Test-Driven Development is not just about testing; it's about design.
    - *Process*: Write the test *before* the implementation. This forces you to design the API from the user's perspective.
    - *Discipline*: Write enough product code to pass the test, then refactor.
- **C++ Core Guidelines Adherence**:
    - **RAII**: No raw `new`/`delete`. Use smart pointers and stack allocation (R.1).
    - **Modern Features**: Use `auto`, `override`, `nullptr`, and `std::string_view` (P.1).
    - **Safety**: No `reinterpret_cast` (unless parsing bytes) or `const_cast`.
    - **Contracts**: Use `[[nodiscard]]` for error codes and `noexcept` for optimization.

## Design Philosophy (HTTP Server/Client Interfaces)

### Interface Design
- **Plug-and-Play Architecture**: Must be able to swap HTTP/1.1 ↔ HTTP/2 server/client without changing business logic
- **Minimal Interfaces**: Only essential methods (get, post) - add features when actually needed, not speculatively
- **No Ivory Tower**: Prefer simple factory functions over complex DI frameworks
- **TDD-Driven APIs**: Write tests first to discover the right interface, not design in isolation

### Thread Architecture for Cloud Native
- **Resource Constraints**: Design for 2 CPU cores per application container (4 total per pod including istio sidecar)
- **Horizontal Scaling**: Scale with more pods (HPA), not more threads per pod
- **Thread Roles**:
  - **Network Threads**: nghttp2 io_context threads (minimal count)
  - **Worker Threads**: Business logic, FSM state (1-2 threads for 2-core pod)
  - **IO Service Threads**: HTTP/DB/Cache operations (1 thread for 2-core pod)
- **Stateful Routing**: I/O responses must return to the SAME worker thread that initiated request (FSM continuity)

### HTTP Client Design
- **Connection Multiplexing**: HTTP/2 clients reuse connections (30s idle timeout, hardcoded for now)
- **Per-Thread Clients**: Each IO service thread owns its own HTTP clients (no sharing, no locks)
- **Simple Callbacks**: Callback captures thread ID, posts result back to originating worker thread
- **No C++20**: Max C++17 (coroutines deferred until time permits learning)

### Configuration Strategy
- **Hardcode Now**: All timeouts, thread counts, connection params hardcoded initially
- **Configuration Later**: Introduce config module (YAML/JSON) after core interfaces stabilize
- **Pragmatic Staging**: Avoid premature configuration complexity

### What We Avoid
- ❌ Complicated dependency injection frameworks
- ❌ Over-engineered abstractions (template metaprogramming, type erasure unless essential)
- ❌ Premature optimization (measure first, optimize second)
- ❌ Monolithic pods (keep resource limits tight, scale horizontally)

## Software Longevity Over Feature Completion

### Core Principle
**Build for the long term, not for quick wins.** Code spends more time being maintained than being written.

### Longevity-First Thinking
- **Maintainability > Speed**: A feature that ships fast but breaks in 6 months is a failure
- **Clarity > Cleverness**: Simple code that anyone can understand beats clever code that only experts can modify
- **Testability > Coverage**: Tests that validate behavior are more valuable than tests that hit 100% coverage
- **Extensibility > Completion**: Leaving clean extension points is better than forcing all features into v1

### Smart Pointer Discipline
- **NEVER use C-style pointers** (, , etc.) in explanations or code
- **ALWAYS use smart pointers**:
  -  for exclusive ownership
  -  for shared ownership
  -  for non-owning references
  - Raw references () ONLY for function parameters where ownership is clear
- **Rationale**: Memory safety, clear ownership semantics, no manual delete, RAII compliance

### What Longevity
