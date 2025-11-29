# Astra

**Astra** is a high-performance, production-ready C++ framework designed for building scalable microservices. It provides a robust set of libraries and abstractions for HTTP/1.1 and HTTP/2 communication, database interactions, state machine management, and observability.

## 🚀 Key Features

*   **High-Performance Networking**:
    *   **HTTP/1.1**: Asynchronous server and client based on **Boost.Beast**.
    *   **HTTP/2**: Full HTTP/2 support using **nghttp2-asio** with a unified router abstraction.
*   **Database Integrations**:
    *   **MongoDB**: Modern C++ client wrapper with connection pooling and CRUD operations.
    *   **Redis**: High-performance Redis client using `redis-plus-plus` and `hiredis`.
*   **Distributed Systems**:
    *   **Zookeeper**: Client for distributed coordination and configuration management.
*   **Observability**:
    *   **Prometheus**: Built-in metrics collection and exposition.
    *   **Structured Logging**: High-performance asynchronous logging.
*   **State Machines**:
    *   Support for **HFSM2** (Hierarchical Finite State Machine) and **Boost.MSM**.
*   **Quality Assurance**:
    *   Integrated **Valgrind** support (MemCheck, Helgrind, Massif) for memory and thread safety.
    *   Comprehensive Unit Testing with **CTest**.
    *   Dual Compiler Support (**GCC** & **Clang**).

---

## 🛠️ Prerequisites

The project is designed to run inside a **Dockerized Development Environment** to ensure consistent dependencies.

*   **Docker**: Ensure Docker is installed and running on your host machine.

---

## 🏁 Quick Start

### 1. Start the Development Container
All development and building should happen inside the `astra` container.

```bash
# Build the Docker image
docker build --network=host -t astrabuilder:v1 -f devenv/Dockerfile devenv

# Run the container
docker run -it --name astra -v $(pwd):/app/astra astrabuilder:v1 bash
```

### 2. Enter the Container
```bash
docker exec -it astra bash
cd /app/astra
```

### 3. Build the Project
You can build using **GCC** (default) or **Clang**.

#### Option A: Build with GCC (Default)
```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)
```

#### Option B: Build with Clang
```bash
# Configure
cmake -S . -B build_clang -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build_clang -j$(nproc)
```

---

## 🧪 Testing & Verification

Astra enforces strict quality gates using CTest and Valgrind.

### Run Unit Tests
```bash
cd build
ctest --output-on-failure
```

### Run Memory & Thread Safety Checks
We use Valgrind to detect memory leaks and race conditions.

```bash
# Memory Leak Detection
make test_memcheck

# Thread Error Detection (Race Conditions)
make test_helgrind

# Heap Profiling
make test_massif
```

---

## 📂 Project Structure

| Directory | Description |
| :--- | :--- |
| `http1.1/` | HTTP/1.1 Server & Client implementation (Boost.Beast). |
| `http2/` | HTTP/2 Server & Client implementation (nghttp2). |
| `router/` | Unified request routing and middleware layer. |
| `mongoclient/` | MongoDB C++ driver wrapper. |
| `redisclient/` | Redis client wrapper. |
| `zookeeperclient/` | Zookeeper client wrapper. |
| `prometheus_client/` | Metrics collection library. |
| `logger/` | Asynchronous structured logging. |
| `hfsm/` | Hierarchical Finite State Machine integration. |
| `devenv/` | Docker environment configuration. |

---

## 📝 Development Guidelines

1.  **Code Style**: Follow standard C++17/20 practices.
2.  **Testing**: Every new feature must have a corresponding unit test in `tests/`.
3.  **Memory Safety**: Run `test_memcheck` before committing to ensure zero leaks.
4.  **Compilers**: Verify builds on both GCC and Clang to ensure portability.

---

## 📜 License

This project is licensed under the MIT License.