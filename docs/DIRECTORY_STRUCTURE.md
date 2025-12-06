# Physical Directory Structure & Design

> **Status**: Proposed / In-Progress
> **Architectural Authority**: Herb Sutter (Physical Design), Robert C. Martin (Screaming Architecture)

## Overview

This document defines the **Physical Design** of the Astra codebase. The directory structure is not merely organizational; it is a strict representation of the **Dependency Graph**.

## The "Expert Consensus" Model

Our structure is derived from the combined philosophies of C++ and Software Architecture experts:

1.  **Herb Sutter (Physical Design)**:
    *   **Rule**: The directory structure must enforce a Directed Acyclic Graph (DAG).
    *   **Constraint**: Lower layers (`libs/core`) **MUST NOT** depend on higher layers (`libs/net`).
    *   **Goal**: Parallelizable builds, no circular dependencies, and clear physical modularity.

2.  **Robert C. Martin (Screaming Architecture)**:
    *   **Rule**: The top-level structure must scream "What the system does" (Policy), not "What framework it uses" (Mechanism).
    *   **Constraint**: Business logic (`apps/`) must be separated from infrastructure details (`libs/`).

3.  **Jason Turner / Scott Meyers**:
    *   **Rule**: Logical grouping of related components (e.g., all Data Clients in one place) for discoverability and interface clarity.

## Directory Layout

```text
astra/
├── apps/                       # [Policy] The "Screaming" part. Business Logic.
│   └── uri_shortener/          # Example Microservice. Depends on libs/net, libs/data.
│
├── libs/                       # [Mechanism] Reusable Components.
│   │
│   ├── core/                   # [Level 0] Foundation. NO internal dependencies.
│   │   ├── concurrency/        # Thread pools, Executors
│   │   ├── config/             # Configuration loading
│   │   ├── exception/          # Exception hierarchy
│   │   ├── observability/      # Logging, Metrics, Tracing
│   │   ├── outcome/            # Error handling types
│   │   ├── uuid_generator/     # UUID utils
│   │   └── json/               # JSON parsing utils
│   │
│   ├── net/                    # [Level 1] Networking. Depends on core.
│   │   ├── http/
│   │   │   ├── v1/             # HTTP/1.1 implementation
│   │   │   └── v2/             # HTTP/2 implementation
│   │   ├── router/             # URL Routing
│   │   └── server/             # Generic Server scaffolding
│   │
│   ├── data/                   # [Level 1] Data Access. Depends on core.
│   │   ├── mongo_client/
│   │   ├── redis_client/
│   │   └── zookeeper_client/
│   │
│   └── fsm/                    # [Level 1] Logic/State. Depends on core.
│       ├── boost_msm/
│       └── hfsm/
│
├── tools/                      # Build scripts, Dockerfiles
├── cmake/                      # CMake modules
└── CMakeLists.txt              # Root build definition
```

## Dependency Rules (The "Sutter Test")

To maintain build health and architectural integrity, the following dependency rules are **enforced**:

1.  **`apps/`** can depend on **`libs/`**.
2.  **`libs/`** can **NEVER** depend on **`apps/`**.
3.  **`libs/net`**, **`libs/data`**, **`libs/fsm`** can depend on **`libs/core`**.
4.  **`libs/core`** can **NEVER** depend on `net`, `data`, or `fsm`.
5.  **Siblings** in `libs/` (e.g., `net` vs `data`) should ideally be independent. If dependency is needed, strict review is required to prevent cycles.

## Build System Implications

-   **CMake**: Each directory (`apps`, `libs`, `core`, `net`) will have its own `CMakeLists.txt`.
-   **Targets**: Libraries will be defined as `astra::core`, `astra::net`, etc., to allow precise linking.
-   **Includes**: Include paths will reflect the physical structure (e.g., `#include "astra/net/http/v2/Server.h"`).
