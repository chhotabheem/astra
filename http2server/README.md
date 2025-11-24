# HTTP/2 Server Library

A production-ready, high-performance HTTP/2 server library for C++ with a clean abstraction layer that allows easy replacement of the underlying network engine.

## Features

- **High Performance**: 425K+ requests/second (42x the 10K TPS target)
- **HTTP/2 Native**: Full HTTP/2 support via nghttp2-asio
- **Clean API**: Simple, elegant interface using Pimpl idiom
- **Backend Agnostic**: Easy to swap network engines
- **Async I/O**: Non-blocking request handling
- **Multi-threaded**: Configurable worker thread pool
- **Production Ready**: Structured logging, error handling, RAII

## Quick Start

### Basic Example

```cpp
#include <http2server/server.hpp>
#include <http2server/request.hpp>
#include <http2server/response.hpp>

int main() {
    // Create server: address, port, threads
    http2server::Server server("0.0.0.0", "8080", 4);

    // Register GET handler
    server.handle("GET", "/", [](const auto& req, auto& res) {
        res.set_status(200);
        res.set_header("content-type", "text/plain");
        res.write("Hello, HTTP/2 World!");
        res.close();
    });

    // Register POST handler
    server.handle("POST", "/echo", [](const auto& req, auto& res) {
        res.set_status(200);
        res.set_header("content-type", "application/json");
        res.write(req.body());
        res.close();
    });

    // Start server (blocking)
    server.run();
    return 0;
}
```

## Performance Benchmarks

### Benchmark Setup

**Hardware**: Docker container on Linux host  
**Tool**: `h2load` (nghttp2 HTTP/2 benchmarking tool)  
**Configuration**: 
- 100 concurrent connections
- 100,000 total requests
- 10 streams per connection

### Command

```bash
h2load -n 100000 -c 100 -m 10 http://localhost:8080/
```

### Results

```
finished in 235.19ms, 425189.95 req/s, 16.65MB/s
requests: 100000 total, 100000 started, 100000 done, 100000 succeeded, 0 failed, 0 errored, 0 timeout
status codes: 100000 2xx, 0 3xx, 0 4xx, 0 5xx
traffic: 3.92MB (4105500) total, 296.00KB (303100) headers (space savings 95.34%), 1.91MB (2000000) data
                     min         max         mean         sd        +/- sd
time for request:      740us      3.32ms      1.98ms       335us    79.66%
time for connect:     1.66ms     15.55ms      9.56ms      5.56ms    55.00%
time to 1st byte:     3.83ms     18.10ms     11.87ms      5.70ms    48.00%
req/s           :    4267.58     4651.55     4441.52      122.20    59.00%
```

**Key Metrics**:
- **Throughput**: **425,189 requests/second**
- **Latency (mean)**: 1.98ms
- **Success Rate**: 100%
- **Header Compression**: 95.34% space savings

### Running Your Own Benchmarks

1. **Install h2load**:
   ```bash
   apt-get install nghttp2-client
   ```

2. **Start the example server**:
   ```bash
   ./build/http2server/simple_server
   ```

3. **Run benchmark**:
   ```bash
   # Basic benchmark
   h2load -n 10000 -c 10 http://localhost:8080/

   # Stress test
   h2load -n 1000000 -c 100 -m 10 http://localhost:8080/
   ```

4. **Test with curl**:
   ```bash
   curl -v --http2-prior-knowledge http://localhost:8080/
   ```

## Building

### Prerequisites

- CMake 3.15+
- C++17 compiler (GCC 9+, Clang 10+)
- Boost (system, thread, chrono)
- nghttp2-asio (fetched automatically)

### Build Commands

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build -j4

# Build specific targets
cmake --build build --target http2server
cmake --build build --target simple_server
```

### Inside Docker

```bash
docker exec astra bash -c "cd astra/build && cmake .. && make http2server -j4"
```

## Running Tests

### CTest

```bash
cd build
ctest --output-on-failure
```

### Manual Testing

```bash
# Run example server
./build/http2server/simple_server

# In another terminal
curl --http2-prior-knowledge http://localhost:8080/
```

## API Reference

### Server

```cpp
class Server {
public:
    // Constructor: address, port, worker threads
    Server(const std::string& address, const std::string& port, int threads = 1);
    
    // Register handler for method + path
    void handle(const std::string& method, const std::string& path, Handler handler);
    
    // Start server (blocking)
    void run();
    
    // Stop server gracefully
    void stop();
};
```

### Request

```cpp
class Request {
public:
    const std::string& method() const;
    const std::string& path() const;
    const std::string& header(const std::string& key) const;
    const std::string& body() const;
};
```

### Response

```cpp
class Response {
public:
    void set_status(int code);
    void set_header(const std::string& key, const std::string& value);
    void write(const std::string& data);
    void close();
};
```

## Architecture

### Directory Structure

```
http2server/
├── include/http2server/
│   ├── server.hpp          # Public Server API
│   ├── request.hpp         # Public Request API
│   └── response.hpp        # Public Response API
├── src/
│   ├── server.cpp          # Server implementation (Pimpl)
│   ├── request.cpp         # Request implementation
│   ├── response.cpp        # Response implementation
│   ├── request_impl.hpp    # Internal Request::Impl
│   ├── response_impl.hpp   # Internal Response::Impl
│   └── backend/nghttp2/    # nghttp2-asio backend
│       ├── server_impl.hpp
│       └── server_impl.cpp
├── examples/
│   └── simple_server.cpp   # Example application
└── nghttp2/
    └── CMakeLists.txt      # nghttp2-asio FetchContent
```

### Design Patterns

- **Pimpl Idiom**: Hides implementation details, enables backend swapping
- **RAII**: Automatic resource management
- **Callback-based**: Async request handling via lambdas

## Dependencies

- **nghttp2-asio**: HTTP/2 implementation (auto-fetched)
- **Boost**: ASIO, system, thread, chrono
- **logger**: Existing logging library (from project)

## Configuration

### Thread Pool

```cpp
// Single-threaded
Server server("0.0.0.0", "8080", 1);

// Multi-threaded (recommended for production)
Server server("0.0.0.0", "8080", std::thread::hardware_concurrency());
```

### Binding

```cpp
// Localhost only
Server server("127.0.0.1", "8080");

// All interfaces
Server server("0.0.0.0", "8080");
```

## Logging

The library uses the existing `logger` library for structured logging:

```
[INFO] NgHttp2Server initialized with 4 threads
[INFO] Server listening on 0.0.0.0:8080
[INFO] Server stopped
```

## Limitations & Future Work

### Current Limitations

- **No TLS support**: Requires external TLS termination (Ingress, Istio)
- **No signal handling**: Must be implemented in application
- **No health endpoints**: Application must implement `/health` routes
- **Response buffering**: Entire response buffered before sending (no streaming)

### Planned Features

- [ ] Signal handling (SIGTERM/SIGINT)
- [ ] Health check endpoints
- [ ] Environment-based configuration
- [ ] Prometheus metrics
- [ ] Connection draining
- [ ] Request body size limits
- [ ] TLS/SSL support
- [ ] Streaming responses

## Troubleshooting

### Build Issues

**Error**: `nghttp2-asio not found`
```bash
# Clean and rebuild
rm -rf build
cmake -B build -S .
cmake --build build
```

**Error**: `Boost not found`
```bash
# Install Boost
apt-get install libboost-all-dev
```

### Runtime Issues

**Error**: `Address already in use`
```bash
# Check if port is in use
lsof -i :8080

# Kill existing process
kill -9 <PID>
```

## Contributing

1. Follow existing code style (no comments, self-explanatory code)
2. Use `m_` prefix for member variables
3. Maintain Pimpl idiom for public classes
4. Add benchmarks for performance-critical changes

## License

[Project License]

## See Also

- [Cloud-Native Kubernetes Analysis](../cloud_native_analysis.md)
- [Implementation Plan](../implementation_plan.md)
- [Walkthrough](../walkthrough.md)
