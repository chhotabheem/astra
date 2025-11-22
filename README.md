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
    ├── mongodriver/    # MongoDB driver build configuration
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

## Key Features

- **Version Control**: The MongoDB C++ driver version is managed via `mongoclient/mongodriver/CMakeLists.txt`.
- **Automatic Build**: The driver is automatically downloaded and built from source when you compile the project.
- **Dockerized**: Full development environment provided via Docker.

## Documentation

For detailed Docker setup, compilation instructions, and usage information, see [imagebuilder/README.md](imagebuilder/README.md).