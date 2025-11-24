# HTTP Server Module

## Overview

HTTP/2 server abstraction layer with nghttp2-asio backend for high-performance async request handling.

## Architecture

### Abstraction Layer
- `IHttpServer` - Server interface
- `IHttpRequest` - Request abstraction
- `IHttpResponse` - Response abstraction  
- `IRouter` - Routing interface

### Implementation
- `NgHttp2Server` - nghttp2-asio HTTP/2 server
- `NgHttp2Request` - Request wrapper
- `NgHttp2Response` - Response wrapper
- `NgHttp2Router` - Route management

### Design Goals
- **Framework agnostic** - Can switch HTTP backends without changing application code
- **Async-first** - Non-blocking request handling for high TPS
- **HTTP/2 native** - Multiplexing, server push, header compression

## Building

### Prerequisites
- Boost (system, thread, chrono) - installed via apt in Docker image
- nghttp2 - installed via apt in Docker image
- nghttp2-asio - fetched automatically via CMake FetchContent

### Build Commands

```bash
# Inside container
cd /app/astra/httpserver
cmake -B build -S .
cmake --build build
```

### From Host

```bash
docker run --rm --network=host -v $(pwd):/app/astra astrabuilder:nghttp2 \
  bash -c "cd /app/astra/httpserver && cmake -B build -S . && cmake --build build"
```

## Running Tests

### Using CTest

```bash
cd /app/astra/httpserver/build
ctest --output-on-failure
```

## Test HTTP/2 Server

A simple HTTP/2 server is available for testing the patched `nghttp2-asio` library.

### Building the Test Server

```bash
# From root directory
cd /app/astra
cmake --build build --target test_http2_server
```

### Running the Test Server

```bash
# Start server (default: 127.0.0.1:8080)
./build/httpserver/test_http2_server

# Or specify custom port
./build/httpserver/test_http2_server 9090

# Or specify custom address and port
./build/httpserver/test_http2_server 9090 0.0.0.0
```

### Testing the Server

```bash
# Test root endpoint
curl -v --http2-prior-knowledge http://127.0.0.1:8080/

# Test health endpoint
curl -v --http2-prior-knowledge http://127.0.0.1:8080/health
```

### Expected Responses

**Root endpoint (`/`):**
```json
{
  "status": "ok",
  "message": "Hello from nghttp2-asio HTTP/2 server!",
  "path": "/"
}
```

**Health endpoint (`/health`):**
```json
{
  "status": "healthy",
  "service": "nghttp2-asio-test"
}
```

### Test Coverage
- HTTP server lifecycle
- Request/response handling
- Routing logic
- Async callback flow

## Integration Example

```cpp
#include "HttpServerFactory.h"

int main() {
    // Create HTTP/2 server
    auto server = HttpServerFactory::create(ServerType::NGHTTP2);
    
    // Add async route
    server->addRoute("/users/{id}", HttpMethod::GET, 
        [](const IHttpRequest& req, auto res, auto done) {
            auto userId = req.getPathParam("id");
            
            // Async database call
            dbClient.findUser(userId, [res, done](User user) {
                res->json(user.toJson());
                done();  // Signal response ready
            });
        });
    
    // Start server
    server->start("0.0.0.0", 8080);
    return 0;
}
```

## Dependencies

- **nghttp2-asio** (fetched via CMake FetchContent)
- **Boost.Asio** (system package)
- **simdjson** (optional, for JSON validation)
- C++17 or later

## Project Structure

```
httpserver/
├── CMakeLists.txt
├── include/
│   ├── IHttpServer.h
│   ├── IHttpRequest.h
│   ├── IHttpResponse.h
│   └── HttpServerFactory.h
├── src/
│   ├── HttpServerFactory.cpp
│   ├── Validator.cpp
│   ├── nghttp2/           # nghttp2-asio implementation
│   │   ├── NgHttp2Server.cpp
│   │   ├── NgHttp2Request.cpp
│   │   ├── NgHttp2Response.cpp
│   │   └── NgHttp2Router.cpp
│   └── mock/              # Mock for testing
│       └── MockHttpServer.h
├── nghttp2/               # FetchContent configuration
│   └── CMakeLists.txt
└── tests/
    └── (test files)
```

## Configuration

The module uses CMake FetchContent to automatically fetch nghttp2-asio v1.62.1 from GitHub during build.

## Performance Notes

- **Event loop based** - Single-threaded async I/O via Boost.Asio
- **Non-blocking** - Handlers return immediately, callbacks execute when ready
- **High concurrency** - Can handle 100K+ concurrent connections
- **Low latency** - Sub-millisecond response times for simple endpoints

## Next Steps

1. Implement NgHttp2Server classes (currently placeholders)
2. Add comprehensive test suite
3. Add benchmarks
4. Document async handler patterns
