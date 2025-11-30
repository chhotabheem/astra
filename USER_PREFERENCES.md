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
- **Tools**: Use `nproc/2` to preserve system responsiveness.

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
