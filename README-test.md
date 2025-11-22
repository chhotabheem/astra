# Astra Project

This project contains a MongoDB C++ client library with Docker support for development and testing.

## Project Structure

```
astra/
├── imagebuilder/       # Docker image build files
│   ├── Dockerfile      # Docker image definition
│   └── README.md       # Detailed Docker setup and usage instructions
└── mongoclient/        # MongoDB C++ client implementation
    ├── CMakeLists.txt
    ├── IMongoClient.h
    ├── MongoClient.h
    ├── MongoClient.cpp
    ├── main.cpp
    └── tests/
```

## Quick Start

### 1. Build the Docker Image

```bash
cd imagebuilder
docker build -t mongo-test .
```

### 2. Run the Container

```bash
# From project root
docker rm -f prayag
docker run -d --name prayag -v $(pwd):/app/astra mongo-test tail -f /dev/null
```

### 3. Compile and Test

```bash
docker exec -it prayag /bin/bash -c "cd /app/astra/mongoclient && mkdir -p build && cd build && cmake .. && cmake --build . && ctest --output-on-failure && ./mongo_app"
```

## Documentation

For detailed Docker setup, compilation instructions, and usage information, see [imagebuilder/README.md](imagebuilder/README.md).

## Components

- **imagebuilder**: Contains the Dockerfile and comprehensive documentation for setting up the development environment
- **mongoclient**: MongoDB C++ client library implementation with tests
