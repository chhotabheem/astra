# Astra Server

## Overview

Main HTTP/2 API server application that combines the httpserver, logger, and mongoclient modules to provide a production-ready REST API service.

## Architecture

```
[External Clients]
    ↓ HTTP/2 REST
[Astra Server] (this module)
    ↓
├─ HTTP Server (nghttp2-asio)
├─ Logger (spdlog)
└─ MongoDB Client
    ↓ HTTP/2 (internal)
[DB Service] (separate microservice)
    ↓
[MongoDB]
```

## Building

### Inside Container

```bash
cd /app/astra
cmake -B build -S .
cmake --build build --target astra_server
```

### From Host

```bash
docker run --rm --network=host -v $(pwd):/app/astra astrabuilder:nghttp2 \
  bash -c "cd /app/astra && cmake -B build -S . && cmake --build build --target astra_server"
```

## Running

```bash
cd /app/astra/build
./server/astra_server
```

## Configuration

(Configuration details to be added as the server is implemented)

## API Endpoints

(API documentation to be added as endpoints are implemented)

## Dependencies

- **httpserver** - HTTP/2 server with nghttp2-asio
- **logger** - Async logging
- **mongoclient** - MongoDB client (for DB service communication)

## Development

### Running Tests

```bash
cd /app/astra/build
ctest --output-on-failure
```

### Hot Reload Development

```bash
# In one terminal: watch for changes and rebuild
while true; do
    inotifywait -r -e modify /app/astra/server
    cmake --build build --target astra_server
done

# In another terminal: run server
./build/server/astra_server
```

## Deployment

(Deployment instructions to be added)

## Project Structure

```
server/
├── CMakeLists.txt
├── main.cpp           # Application entry point
└── (additional files as implemented)
```
