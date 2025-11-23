# MongoDB C++ Client

## Overview

MongoDB client library for the Astra project. Provides async MongoDB operations with connection pooling.

## Building

### Inside Container

```bash
cd /app/astra/mongoclient
cmake -B build -S .
cmake --build build
```

### From Host

```bash
docker run --rm --network=host -v $(pwd):/app/astra astrabuilder:nghttp2 \
  bash -c "cd /app/astra/mongoclient && cmake -B build -S . && cmake --build build"
```

**Note:** The MongoDB C++ driver is automatically downloaded and built from source during the CMake configuration step.

## Running Tests

### Using CTest

```bash
cd /app/astra/mongoclient/build
ctest --output-on-failure
```

**Expected output:**
```
Test project /app/astra/mongoclient/build
    Start 1: MongoClientTest
1/1 Test #1: MongoClientTest ..................   Passed    0.05 sec

100% tests passed, 0 tests failed out of 1
```

### Running Test Executable Directly

```bash
cd /app/astra/mongoclient/build
./test_mongoclient
```

### Running the Demo Application

```bash
cd /app/astra/mongoclient/build
./mongo_app
```

**Note:** The application attempts to connect to MongoDB. Ensure a MongoDB instance is reachable at the configured address.

## Configuration

### MongoDB Driver Version

The MongoDB C++ driver version is managed in `mongodriver/CMakeLists.txt`:

```cmake
set(MONGO_CXX_DRIVER_VERSION "r4.1.4")
```

## Integration Example

```cpp
#include "MongoClient.h"

int main() {
    MongoClient client("mongodb://localhost:27017");
    
    // Async insert
    client.insert("mydb", "users", userDocument, 
        [](bool success) {
            if (success) {
                std::cout << "Insert successful\n";
            }
        });
    
    // Async find
    client.findOne("mydb", "users", {{"_id", userId}},
        [](const bson::Document& doc) {
            std::cout << "Found: " << doc.toJson() << "\n";
        });
    
    return 0;
}
```

## Dependencies

- **MongoDB C++ Driver** r4.1.4 (fetched automatically via CMake FetchContent)
- **MongoDB C Driver** 2.1.2 (fetched automatically as dependency)
- **logger** module
- C++17 or later

## Project Structure

```
mongoclient/
├── CMakeLists.txt
├── mongodriver/           # MongoDB driver build configuration
│   └── CMakeLists.txt
├── IMongoClient.h         # Interface definition
├── MongoClient.h          # Implementation header
├── MongoClient.cpp        # Implementation
├── main.cpp               # Demo application
└── tests/
    └── test_mongoclient.cpp  # Unit tests
```

## Troubleshooting

### Build Issues

If the MongoDB driver fails to download:
- Ensure the container has network access (`--network=host`)
- Check GitHub is accessible
- Verify CMake FetchContent is working

### Connection Issues

If the application can't connect to MongoDB:
- Verify MongoDB is running
- Check the connection string
- Ensure network connectivity between container and MongoDB

## Development Workflow

### Quick Build and Test

```bash
# From project root
docker run --rm --network=host -v $(pwd):/app/astra astrabuilder:nghttp2 \
  bash -c "cd /app/astra/mongoclient && rm -rf build && cmake -B build -S . && cmake --build build && cd build && ctest --output-on-failure"
```

### Interactive Development

```bash
# Start container
./imagebuilder/build_container.py --up

# Enter container
docker exec -it astra /bin/bash

# Work in mongoclient directory
cd /app/astra/mongoclient
cmake -B build -S .
cmake --build build
cd build && ctest
```
