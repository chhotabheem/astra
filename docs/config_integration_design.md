# Config Integration Design - Implementation Guide

> **Purpose:** This document enables ANY AI model or developer to implement config integration across all astra libraries. It is self-contained and requires no prior context.

---

## Quick Reference - Complete File Manifest

### NEW Files to Create
| Path | Type | Purpose |
|------|------|---------|
| `apps/uri_shortener/config.json` | JSON | Runtime configuration file |
| `apps/uri_shortener/tests/test_config_loading.cpp` | Test | Verify config loads correctly |

### Files to MODIFY
| Path | Change Type | Description |
|------|-------------|-------------|
| `apps/uri_shortener/CMakeLists.txt` | Add dependency | Link against `config` library |
| `apps/uri_shortener/main.cpp` | Refactor | Use ConfigProvider instead of hardcoded values |
| `libs/core/observability/include/obs/Observability.h` | Add methods | `set_sample_rate()`, `set_log_level()` |
| `libs/core/observability/src/Observability.cpp` | Implement | Hot reload methods |
| `libs/core/config/include/ConfigStructs.h` | Optional | Add `operator==` for BootstrapConfig |

### NO Changes Needed
| Path | Reason |
|------|--------|
| `libs/core/config/*` | Already complete and functional |
| `libs/data/mongo_client/*` | Already accepts URI via constructor |
| `libs/data/redis_client/*` | Already accepts URI via constructor |
| `libs/net/http/v2/server/*` | Already accepts address/port/threads |
| `libs/net/http/v1/server/*` | Already accepts address/port/threads |

---

## New Classes/Interfaces Required

### None Required for P1

The existing infrastructure is sufficient:
- `ConfigProvider` ✅ exists
- `IConfigSource` ✅ exists  
- `IConfigParser` ✅ exists
- `Config` struct ✅ exists with full schema

### P2 - Optional Interfaces (for cleaner hot reload)

If desired, these interfaces can be added later:

```cpp
// File: libs/core/config/include/IReloadable.h (OPTIONAL - P2)
#pragma once

namespace config {

/// Interface for components that support hot config reload
template<typename T>
class IReloadable {
public:
    virtual ~IReloadable() = default;
    virtual void on_config_update(const T& new_config) = 0;
};

} // namespace config
```

**Decision:** Not needed for P1. Direct callbacks via `provider.onUpdate()` are sufficient.

---

## Architecture Decisions

### 1. No Global Singleton
```
REJECTED: ConfigProvider::instance().get()
APPROVED: ConfigProvider created in main(), values passed explicitly
```

### 2. Hybrid Injection Pattern
```
Bootstrap Config (Day 0):
  - Passed directly at construction
  - Cannot change without restart
  - Examples: port, mongodb_uri, worker_threads

Operational/Runtime Config (Day 1/2):
  - Hot reloadable via callbacks
  - Examples: log_level, sample_rate, feature_flags
```

### 3. Lock-Free Config Access
```cpp
// ConfigProvider uses atomic shared_ptr
std::shared_ptr<const Config> get() const {
    return std::atomic_load(&m_config);  // No mutex on read path
}
```

---

## 🚨 Architectural Issue: main() Knows Too Much

### Current Problem

```cpp
// main.cpp knows:
// - RedisClient (infrastructure)
// - MongoClient (infrastructure)  
// - Http1Server (infrastructure)
// - Http2Server (infrastructure)
// - RedisUriRepository (repository)
// - MongoUriRepository (repository)
// - UriService (business)
// - UriController (presentation)
// - ConfigProvider (config)

// This violates Dependency Inversion!
```

### Uncle Bob's Clean Architecture Says:

```
main() should ONLY know:
├── CompositionRoot (wires everything)
└── Server.run()

Business logic should ONLY know:
├── IUriRepository (interface)
└── Router.dispatch()
```

### Proposed Fix: Composition Root Pattern

**Create new file:** `apps/uri_shortener/src/CompositionRoot.h`

```cpp
#pragma once
#include <memory>

namespace config { class ConfigProvider; struct Config; }
namespace router { class Router; }

namespace uri_shortener {

class Application {
public:
    static std::unique_ptr<Application> create(const config::Config& cfg);
    
    void configure_routes(router::Router& router);
    void shutdown();
    
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace uri_shortener
```

**Create new file:** `apps/uri_shortener/src/CompositionRoot.cpp`

```cpp
#include "CompositionRoot.h"
#include "IUriRepository.h"
#include "RedisUriRepository.h"
#include "MongoUriRepository.h"
#include "UriService.h"
#include "UriController.h"
#include "RedisClient.h"
#include "MongoClient.h"
#include "ConfigStructs.h"

namespace uri_shortener {

class Application::Impl {
public:
    std::shared_ptr<redisclient::RedisClient> redis;
    std::shared_ptr<mongoclient::MongoClient> mongo;
    std::shared_ptr<IUriRepository> repository;
    std::shared_ptr<UriService> service;
    std::shared_ptr<UriController> controller;
};

std::unique_ptr<Application> Application::create(const config::Config& cfg) {
    auto app = std::make_unique<Application>();
    app->m_impl = std::make_unique<Impl>();
    
    // Infrastructure (knows concrete types)
    app->m_impl->redis = std::make_shared<redisclient::RedisClient>(
        cfg.m_bootstrap.m_database.m_redis_uri
    );
    
    app->m_impl->mongo = std::make_shared<mongoclient::MongoClient>();
    app->m_impl->mongo->connect(cfg.m_bootstrap.m_database.m_mongodb_uri);
    
    // Repository (via interface)
    app->m_impl->repository = std::make_shared<RedisUriRepository>(app->m_impl->redis);
    
    // Business (only knows IUriRepository)
    app->m_impl->service = std::make_shared<UriService>(app->m_impl->repository);
    
    // Presentation
    app->m_impl->controller = std::make_shared<UriController>(app->m_impl->service);
    
    return app;
}

void Application::configure_routes(router::Router& router) {
    router.post("/shorten", [ctrl = m_impl->controller](auto& req, auto& res) {
        ctrl->shorten(req, res);
    });
    
    router.get("/:code", [ctrl = m_impl->controller](auto& req, auto& res) {
        ctrl->redirect(req, res);
    });
}

void Application::shutdown() {
    m_impl->mongo->disconnect();
}

} // namespace uri_shortener
```

### Cleaner main.cpp

```cpp
#include "Http2Server.h"
#include "CompositionRoot.h"
#include "ConfigProvider.h"
#include "filesource/FileConfigSource.h"
#include "parsers/JsonConfigParser.h"

int main(int argc, char* argv[]) {
    // 1. Load config
    auto provider = config::ConfigProvider::create(...).unwrap();
    auto cfg = provider.get();
    
    // 2. Create application (hides all infrastructure details)
    auto app = uri_shortener::Application::create(*cfg);
    
    // 3. Create server (main only knows address/port)
    http2server::Server server(
        cfg->m_bootstrap.m_server.m_address,
        std::to_string(cfg->m_bootstrap.m_server.m_port),
        cfg->m_bootstrap.m_threading.m_worker_threads
    );
    
    // 4. Wire routes (app configures, main doesn't know handlers)
    app->configure_routes(server.router());
    
    // 5. Run
    provider.start();
    server.run();
    
    // 6. Cleanup
    app->shutdown();
}
```

### Updated File Manifest

| Path | Action | Priority |
|------|--------|----------|
| `apps/uri_shortener/src/CompositionRoot.h` | CREATE | P1 |
| `apps/uri_shortener/src/CompositionRoot.cpp` | CREATE | P1 |
| `apps/uri_shortener/main.cpp` | SIMPLIFY | P1 |

---

## Project Overview

- **Repository:** `/home/siddu/astra` (or `/app/astra` in container)
- **Config Library:** `libs/core/config/`
- **Main Application:** `apps/uri_shortener/`
- **Build System:** CMake with presets

---

## Current Problem

The application (`apps/uri_shortener/main.cpp`) has hardcoded values that should come from central configuration:

```cpp
// Line 28 - HARDCODED Redis URI
auto redis = std::make_shared<redisclient::RedisClient>("tcp://127.0.0.1:6379");

// Line 30 - HARDCODED MongoDB URI
mongo->connect("mongodb://localhost:27017");

// Lines 62-63 - HARDCODED ports and thread counts
http1::Server http1_server("0.0.0.0", 8081, 2);
http2server::Server http2_server("0.0.0.0", "8080", 4);
```

---

## Existing Config Infrastructure

### ConfigProvider (Already Exists)
**File:** `libs/core/config/include/ConfigProvider.h`

```cpp
class ConfigProvider {
public:
    static astra::Result<ConfigProvider, std::string> create(
        std::unique_ptr<IConfigSource> source,
        std::unique_ptr<IConfigParser> parser,
        std::shared_ptr<IConfigLogger> logger = nullptr,
        std::shared_ptr<IConfigMetrics> metrics = nullptr
    );
    
    std::shared_ptr<const Config> get() const;  // Thread-safe, lock-free
    void onUpdate(UpdateCallback callback);      // Register for hot reload
    void start();                                // Start watching
    void stop();                                 // Stop watching
};
```

### ConfigStructs (Already Exists)
**File:** `libs/core/config/include/ConfigStructs.h`

```cpp
struct ServerConfig {
    std::string m_address{"0.0.0.0"};
    uint16_t m_port{8080};
};

struct DatabaseConfig {
    std::string m_mongodb_uri;
    std::string m_redis_uri;
};

struct ThreadingConfig {
    size_t m_worker_threads{2};
    size_t m_io_service_threads{1};
};

struct ObservabilityConfig {
    bool m_metrics_enabled{true};
    double m_tracing_sample_rate{0.1};
};

struct Config {
    int m_version{1};
    BootstrapConfig m_bootstrap;      // Day 0 - immutable after start
    OperationalConfig m_operational;  // Day 1 - hot reloadable
    RuntimeConfig m_runtime;          // Day 2 - hot reloadable
};
```

---

## Implementation Tasks

### Task 1: Create config.json file
**Action:** Create new file
**Path:** `apps/uri_shortener/config.json`

```json
{
  "version": 1,
  "bootstrap": {
    "server": {
      "address": "0.0.0.0",
      "port": 8080
    },
    "threading": {
      "worker_threads": 4,
      "io_service_threads": 2
    },
    "database": {
      "mongodb_uri": "mongodb://localhost:27017",
      "redis_uri": "tcp://127.0.0.1:6379"
    },
    "service": {
      "name": "uri-shortener",
      "environment": "development"
    }
  },
  "operational": {
    "logging": {
      "level": "INFO",
      "format": "json",
      "enable_access_logs": true
    },
    "timeouts": {
      "request_ms": 5000,
      "database_ms": 2000,
      "http_client_ms": 3000
    },
    "connection_pools": {
      "mongodb_pool_size": 10,
      "redis_pool_size": 5,
      "http2_max_connections": 100
    },
    "observability": {
      "metrics_enabled": true,
      "tracing_sample_rate": 0.1
    }
  },
  "runtime": {
    "rate_limiting": {
      "global_rps_limit": 100000,
      "per_user_rps_limit": 1000,
      "burst_size": 5000
    },
    "circuit_breaker": {
      "mongodb_threshold": 5,
      "mongodb_timeout_sec": 30,
      "redis_threshold": 3,
      "redis_timeout_sec": 30
    },
    "feature_flags": {
      "enable_caching": true,
      "enable_url_preview": false,
      "compression_enabled": true
    },
    "backpressure": {
      "worker_queue_max": 10000,
      "io_queue_max": 5000
    }
  }
}
```

---

### Task 2: Update uri_shortener CMakeLists.txt
**Action:** Modify file
**Path:** `apps/uri_shortener/CMakeLists.txt`

**Current content (approximate):**
```cmake
add_executable(uri_shortener
    main.cpp
    # ... other files
)

target_link_libraries(uri_shortener
    PRIVATE
        http1server
        http2server
        redis_client
        mongo_client
        router
)
```

**New content:**
```cmake
add_executable(uri_shortener
    main.cpp
    # ... other files
)

target_link_libraries(uri_shortener
    PRIVATE
        http1server
        http2server
        redis_client
        mongo_client
        router
        config          # ADD THIS LINE
        observability   # ADD THIS LINE (for future hot reload)
)

target_include_directories(uri_shortener
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/core/config/include
)

# Copy config.json to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json
    ${CMAKE_CURRENT_BINARY_DIR}/config.json
    COPYONLY
)
```

---

### Task 2b: Create Test File (Optional but Recommended)
**Action:** Create new file
**Path:** `apps/uri_shortener/tests/test_config_loading.cpp`

```cpp
#include <gtest/gtest.h>
#include "ConfigProvider.h"
#include "filesource/FileConfigSource.h"
#include "parsers/JsonConfigParser.h"
#include <fstream>
#include <filesystem>

namespace uri_shortener::test {

class ConfigLoadingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test config file
        test_config_path_ = std::filesystem::temp_directory_path() / "test_config.json";
        std::ofstream ofs(test_config_path_);
        ofs << R"({
            "version": 1,
            "bootstrap": {
                "server": {"address": "0.0.0.0", "port": 9999},
                "threading": {"worker_threads": 2, "io_service_threads": 1},
                "database": {
                    "mongodb_uri": "mongodb://test:27017",
                    "redis_uri": "tcp://test:6379"
                },
                "service": {"name": "test-service", "environment": "test"}
            },
            "operational": {
                "logging": {"level": "DEBUG", "format": "json", "enable_access_logs": true},
                "timeouts": {"request_ms": 1000, "database_ms": 500, "http_client_ms": 1500},
                "connection_pools": {"mongodb_pool_size": 5, "redis_pool_size": 2, "http2_max_connections": 50},
                "observability": {"metrics_enabled": true, "tracing_sample_rate": 1.0}
            },
            "runtime": {
                "rate_limiting": {"global_rps_limit": 1000, "per_user_rps_limit": 100, "burst_size": 500},
                "circuit_breaker": {"mongodb_threshold": 3, "mongodb_timeout_sec": 10, "redis_threshold": 2, "redis_timeout_sec": 10},
                "feature_flags": {"enable_caching": false, "enable_url_preview": false, "compression_enabled": false},
                "backpressure": {"worker_queue_max": 1000, "io_queue_max": 500}
            }
        })";
        ofs.close();
    }
    
    void TearDown() override {
        std::filesystem::remove(test_config_path_);
    }
    
    std::filesystem::path test_config_path_;
};

TEST_F(ConfigLoadingTest, LoadsConfigSuccessfully) {
    auto result = config::ConfigProvider::create(
        std::make_unique<config::FileConfigSource>(test_config_path_),
        std::make_unique<config::JsonConfigParser>()
    );
    
    ASSERT_TRUE(result.is_ok());
    
    auto provider = std::move(result).unwrap();
    auto cfg = provider.get();
    
    EXPECT_EQ(cfg->m_version, 1);
    EXPECT_EQ(cfg->m_bootstrap.m_server.m_port, 9999);
    EXPECT_EQ(cfg->m_bootstrap.m_database.m_mongodb_uri, "mongodb://test:27017");
    EXPECT_EQ(cfg->m_bootstrap.m_database.m_redis_uri, "tcp://test:6379");
    EXPECT_EQ(cfg->m_bootstrap.m_service.m_name, "test-service");
}

TEST_F(ConfigLoadingTest, RejectsInvalidConfig) {
    // Create invalid config
    auto invalid_path = std::filesystem::temp_directory_path() / "invalid_config.json";
    std::ofstream ofs(invalid_path);
    ofs << "{ invalid json }";
    ofs.close();
    
    auto result = config::ConfigProvider::create(
        std::make_unique<config::FileConfigSource>(invalid_path),
        std::make_unique<config::JsonConfigParser>()
    );
    
    EXPECT_FALSE(result.is_ok());
    
    std::filesystem::remove(invalid_path);
}

TEST_F(ConfigLoadingTest, RejectsMissingFile) {
    auto result = config::ConfigProvider::create(
        std::make_unique<config::FileConfigSource>("/nonexistent/config.json"),
        std::make_unique<config::JsonConfigParser>()
    );
    
    EXPECT_FALSE(result.is_ok());
}

} // namespace uri_shortener::test
```

---

### Task 3: Update main.cpp to use ConfigProvider
**Action:** Replace file content
**Path:** `apps/uri_shortener/main.cpp`

```cpp
#include "Http1Server.h"
#include "Http2Server.h"
#include "RedisClient.h"
#include "MongoClient.h"
#include "RedisUriRepository.h"
#include "MongoUriRepository.h"
#include "UriService.h"
#include "UriController.h"
#include "ConfigProvider.h"
#include "filesource/FileConfigSource.h"
#include "parsers/JsonConfigParser.h"
#include <iostream>
#include <thread>
#include <csignal>
#include <filesystem>

using namespace uri_shortener;

std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    try {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 1. Determine config file path
        std::filesystem::path config_path = "config.json";
        if (argc > 1) {
            config_path = argv[1];
        }
        
        std::cout << "Loading configuration from: " << config_path << std::endl;

        // 2. Create ConfigProvider
        auto provider_result = config::ConfigProvider::create(
            std::make_unique<config::FileConfigSource>(config_path),
            std::make_unique<config::JsonConfigParser>()
        );
        
        if (!provider_result.is_ok()) {
            std::cerr << "Failed to load config: " << provider_result.error() << std::endl;
            return 1;
        }
        
        auto provider = std::move(provider_result).unwrap();
        auto cfg = provider.get();
        
        std::cout << "Config loaded: version=" << cfg->m_version 
                  << ", service=" << cfg->m_bootstrap.m_service.m_name << std::endl;

        // 3. Initialize Infrastructure (using config)
        std::cout << "Initializing Infrastructure..." << std::endl;
        
        auto redis = std::make_shared<redisclient::RedisClient>(
            cfg->m_bootstrap.m_database.m_redis_uri
        );
        
        auto mongo = std::make_shared<mongoclient::MongoClient>();
        mongo->connect(cfg->m_bootstrap.m_database.m_mongodb_uri);

        // 4. Initialize Layers
        std::cout << "Initializing Layers..." << std::endl;
        auto redis_repo = std::make_shared<RedisUriRepository>(redis);
        auto mongo_repo = std::make_shared<MongoUriRepository>(mongo);
        auto service = std::make_shared<UriService>(redis_repo);
        auto controller = std::make_shared<UriController>(service);

        // 5. Configure Router
        std::cout << "Configuring Router..." << std::endl;
        auto setup_routes = [&](router::Router& r) {
            r.post("/shorten", [controller](router::IRequest& req, router::IResponse& res) {
                controller->shorten(req, res);
            });
            
            r.get("/:code", [controller](router::IRequest& req, router::IResponse& res) {
                controller->redirect(req, res);
            });
        };

        // 6. Initialize Servers (using config)
        std::cout << "Starting Servers..." << std::endl;
        
        // HTTP/1.1 server on port+1 for health checks
        http1::Server http1_server(
            cfg->m_bootstrap.m_server.m_address,
            cfg->m_bootstrap.m_server.m_port + 1,
            static_cast<int>(cfg->m_bootstrap.m_threading.m_io_service_threads)
        );
        
        // HTTP/2 server on main port
        http2server::Server http2_server(
            cfg->m_bootstrap.m_server.m_address,
            std::to_string(cfg->m_bootstrap.m_server.m_port),
            static_cast<int>(cfg->m_bootstrap.m_threading.m_worker_threads)
        );

        setup_routes(http1_server.router());
        setup_routes(http2_server.router());

        // 7. Start config watching for hot reload
        provider.start();

        // 8. Run servers
        std::thread t1([&] { http1_server.run(); });
        std::thread t2([&] { http2_server.run(); });

        std::cout << "URI Shortener Service Running:" << std::endl;
        std::cout << "  - HTTP/2 (Traffic): http://localhost:" 
                  << cfg->m_bootstrap.m_server.m_port << std::endl;
        std::cout << "  - HTTP/1.1 (Health): http://localhost:" 
                  << (cfg->m_bootstrap.m_server.m_port + 1) << std::endl;

        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "Stopping servers..." << std::endl;
        provider.stop();
        http1_server.stop();
        http2_server.stop();
        
        if (t1.joinable()) t1.join();
        if (t2.joinable()) t2.join();

        std::cout << "Goodbye!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

---

### Task 4: Add obs::set_sample_rate() for hot reload (Optional P2)
**Action:** Add method to observability library
**Path:** `libs/core/observability/include/obs/Observability.h`

**Add after line 59:**
```cpp
/// Update sample rate at runtime (hot reload)
void set_sample_rate(double rate);
```

**Path:** `libs/core/observability/src/Observability.cpp` (or backend)

**Implementation:**
```cpp
void set_sample_rate(double rate) {
    // Implementation depends on backend
    if (g_backend) {
        // Update sampling decision logic
    }
}
```

---

## Verification Steps

### Step 1: Build
```bash
docker exec astra cmake --preset gcc-debug
docker exec astra cmake --build --preset gcc-debug -j2
```

### Step 2: Run with config
```bash
docker exec astra ./build/gcc-debug/apps/uri_shortener/uri_shortener config.json
```

### Step 3: Test hot reload
```bash
# Edit config.json and save - should log "Config change detected"
```

---

## Library-by-Library Status

### Libraries That ALREADY Accept Config (No Changes Needed)
| Library | Constructor | Config Source |
|---------|-------------|---------------|
| `RedisClient` | `RedisClient(const std::string& uri)` | Caller passes from config |
| `MongoClient` | `connect(const std::string& uri)` | Caller passes from config |
| `http2server::Server` | `Server(address, port, threads)` | Caller passes from config |
| `http1::Server` | `Server(address, port, threads)` | Caller passes from config |
| `http2client::ClientConfig` | Struct with defaults | Caller sets fields from config |

### Libraries That Need Hot Reload Support (P2)
| Library | Method Needed | Purpose |
|---------|---------------|---------|
| `observability` | `set_sample_rate(double)` | Change tracing rate without restart |
| `observability` | `set_log_level(string)` | Change log verbosity without restart |

---

## Key Design Decisions

1. **Hybrid Injection Pattern**
   - Bootstrap config (ports, URIs): Direct injection at construction (immutable)
   - Operational/Runtime config: Can be hot-reloaded via callbacks

2. **No Global Accessor**
   - ConfigProvider is created in main() and passed/configured explicitly
   - Libraries don't know about ConfigProvider, they receive values

3. **Single shared_ptr Source**
   - `provider.get()` returns `shared_ptr<const Config>`
   - All readers see consistent snapshot (Lamport-approved)

4. **Config Versioning**
   - `Config::m_version` field for debugging
   - Future: Add to spans for observability

---

## Files Summary

| Path | Action | Priority |
|------|--------|----------|
| `apps/uri_shortener/config.json` | CREATE | P1 |
| `apps/uri_shortener/CMakeLists.txt` | MODIFY | P1 |
| `apps/uri_shortener/main.cpp` | MODIFY | P1 |
| `libs/core/observability/include/obs/Observability.h` | MODIFY | P2 |
| `libs/core/observability/src/Observability.cpp` | MODIFY | P2 |

---

## Estimated Effort

| Phase | Tasks | Time |
|-------|-------|------|
| P1 - uri_shortener integration | config.json, CMakeLists, main.cpp | 1-2 hours |
| P2 - hot reload for observability | set_sample_rate, set_log_level | 1-2 hours |
| P3 - config versioning | Add version to spans | 30 mins |
| **Total** | | **3-5 hours** |
