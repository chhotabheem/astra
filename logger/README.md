# Logger Module

## Overview

Async logging library using [spdlog](https://github.com/gabime/spdlog) for high-performance logging in the Astra project.

## Features

- Async logging for non-blocking performance
- Multiple log levels (trace, debug, info, warn, error, critical)
- File and console output
- Thread-safe
- Header-only interface

## Building

### Inside Docker Container

```bash
# Start the container
./imagebuilder/build_container.py --up

# Enter the container
docker exec -it astra /bin/bash

# Build logger module
cd /app/astra/logger
cmake -B build -S .
cmake --build build
```

### From Host (using Docker)

```bash
docker run --rm -v $(pwd):/app/astra astrabuilder:nghttp2 \
  bash -c "cd /app/astra/logger && cmake -B build -S . && cmake --build build"
```

## Running Tests

### Using CTest

```bash
cd /app/astra/logger/build
ctest --output-on-failure
```

**Expected output:**
```
Test project /app/astra/logger/build
    Start 1: LoggerBasicTest
1/1 Test #1: LoggerBasicTest ..................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 1
```

### Running Test Executable Directly

```bash
cd /app/astra/logger/build
./test_logger
```

## Integration

### CMakeLists.txt

```cmake
add_subdirectory(logger)

target_link_libraries(your_target
    PRIVATE logger
)
```

### C++ Code

```cpp
#include "Logger.h"

int main() {
    auto& logger = Logger::getInstance();
    logger.info("Application started");
    logger.error("Error occurred: {}", error_msg);
    return 0;
}
```

## Dependencies

- **spdlog** v1.12.0 (fetched automatically via CMake FetchContent)
- C++17 or later

## Project Structure

```
logger/
├── CMakeLists.txt       # Build configuration
├── include/
│   └── Logger.h         # Public interface
├── src/
│   └── LoggerImpl.cpp   # Implementation
└── tests/
    └── test_logger.cpp  # Unit tests
```
