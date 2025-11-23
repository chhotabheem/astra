# Astra - High-Performance HTTP/2 API Server

Production-ready C++ HTTP/2 server with async MongoDB client and distributed architecture support.

## Quick Start

### 1. Build the Docker Environment

```bash
./imagebuilder/build_container.py --up
```

This builds the `astrabuilder:nghttp2` Docker image and starts the `astra` container.

### 2. Build the Project

```bash
docker exec -it astra bash -c "cd /app/astra && cmake -B build -S . && cmake --build build"
```

### 3. Run Tests

```bash
docker exec -it astra bash -c "cd /app/astra/build && ctest --output-on-failure"
```

## Project Architecture

```
[External Clients]
    ↓ HTTP/2 REST
[Astra Server]
    ├─ HTTP Server (nghttp2-asio)
    ├─ Logger (spdlog)
    └─ MongoDB Client
        ↓ HTTP/2 (internal)
[DB Service] (separate microservice)
    ↓
[MongoDB]
```

## Project Structure

```
astra/
├── imagebuilder/          # Docker build automation
│   ├── Dockerfile         # Ubuntu 25.10 + Boost + nghttp2
│   ├── build_container.py # Build script (use this!)
│   └── README.md          # Docker setup guide
│
├── logger/                # Async logging module
│   ├── include/Logger.h
│   ├── src/LoggerImpl.cpp
│   ├── tests/
│   └── README.md          # Build & test instructions
│
├── httpserver/            # HTTP/2 server abstraction
│   ├── include/           # IHttpServer, IHttpRequest, IHttpResponse
│   ├── src/nghttp2/       # nghttp2-asio implementation
│   ├── nghttp2/           # FetchContent configuration
│   └── README.md          # Architecture & usage
│
├── mongoclient/           # MongoDB client
│   ├── MongoClient.h/cpp
│   ├── mongodriver/       # MongoDB driver FetchContent
│   ├── tests/
│   └── README.md          # Build & test instructions
│
├── server/                # Main application
│   ├── main.cpp
│   └── README.md          # Server documentation
│
└── CMakeLists.txt         # Root build configuration
```

## Modules

### Logger
Async logging using spdlog for high-performance, non-blocking logging.

**Build & Test:**
```bash
cd /app/astra/logger
cmake -B build -S . && cmake --build build
cd build && ctest --output-on-failure
```

[Full Documentation](logger/README.md)

### HTTP Server
HTTP/2 server abstraction with nghttp2-asio backend. Async-first design for high TPS.

**Key Features:**
- Framework-agnostic interface
- Async request handling
- HTTP/2 native (multiplexing, server push)
- Can handle 100K+ concurrent connections

**Build:**
```bash
cd /app/astra/httpserver
cmake -B build -S . && cmake --build build
```

[Full Documentation](httpserver/README.md)

### MongoDB Client
Async MongoDB client with connection pooling.

**Build & Test:**
```bash
cd /app/astra/mongoclient
cmake -B build -S . && cmake --build build
cd build && ctest --output-on-failure
```

[Full Documentation](mongoclient/README.md)

### Server
Main HTTP/2 API server application combining all modules.

**Build:**
```bash
cd /app/astra
cmake -B build -S . && cmake --build build --target astra_server
```

[Full Documentation](server/README.md)

## Development Workflow

### Interactive Development

```bash
# Start container
./imagebuilder/build_container.py --up

# Enter container
docker exec -it astra /bin/bash

# Build and test
cd /app/astra
cmake -B build -S .
cmake --build build
cd build && ctest
```

### One-Liner Build & Test

```bash
docker run --rm --network=host -v $(pwd):/app/astra astrabuilder:nghttp2 \
  bash -c "cd /app/astra && cmake -B build -S . && cmake --build build && cd build && ctest --output-on-failure"
```

## Testing

### Run All Tests

```bash
cd /app/astra/build
ctest --output-on-failure
```

### Run Module-Specific Tests

```bash
# Logger tests
cd /app/astra/logger/build
ctest --output-on-failure

# MongoDB client tests
cd /app/astra/mongoclient/build
ctest --output-on-failure
```

### Verbose Test Output

```bash
ctest --output-on-failure --verbose
```

## Dependencies

### System Dependencies (in Docker image)
- Ubuntu 25.10
- CMake 3.15+
- GCC 15.2.0 (C++17)
- Boost (system, thread, chrono)
- nghttp2
- OpenSSL, SASL

### CMake FetchContent Dependencies (auto-downloaded)
- **nghttp2-asio** v1.62.1 - HTTP/2 server library
- **MongoDB C++ Driver** r4.1.4 - MongoDB client
- **spdlog** v1.12.0 - Logging library

## Configuration

### Docker Build Script

Configuration is in `imagebuilder/config.json`:

```json
{
    "image_name": "astrabuilder:nghttp2",
    "container_name": "astra",
    "network_mode": "host",
    "build_network": "host"
}
```

### MongoDB Driver Version

Set in `mongoclient/mongodriver/CMakeLists.txt`:

```cmake
set(MONGO_CXX_DRIVER_VERSION "r4.1.4")
```

## Troubleshooting

### Build Fails with DNS Errors

CMake FetchContent requires network access to download dependencies. Ensure:
- Docker build uses `--network=host` (default in build script)
- GitHub is accessible from your network

### Container Name Conflicts

If you get "container name already in use":
```bash
./imagebuilder/build_container.py --stop
./imagebuilder/build_container.py --start
```

### Clean Build

```bash
docker exec -it astra bash -c "cd /app/astra && rm -rf build && cmake -B build -S . && cmake --build build"
```

## Production Deployment

(Deployment documentation to be added as the system matures)

## Contributing

(Contributing guidelines to be added)

## License

See [LICENSE](LICENSE) file for details.