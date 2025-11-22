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

## Quick Start

### 1. Set Up Environment

Use the environment management script to build the Docker image and start the container:

```bash
./imagebuilder/manage_env.py --up
```

### 2. Build and Test

You can compile and test the project inside the container using standard CMake commands:

```bash
docker exec -it prayag /bin/bash -c "mkdir -p build && cd build && cmake /app/astra && cmake --build . && ctest --output-on-failure && ./mongoclient/mongo_app"
```

Or enter the container to work interactively:

```bash
docker exec -it prayag /bin/bash
cd /app/astra
mkdir -p build && cd build
cmake ..
cmake --build .
ctest
```

## Key Features

- **Version Control**: The MongoDB C++ driver version is managed via `mongoclient/mongodriver/CMakeLists.txt`.
- **Automatic Build**: The driver is automatically downloaded and built from source when you compile the project.
- **Dockerized**: Full development environment provided via Docker.

## Documentation

- **Docker Setup**: [imagebuilder/README.md](imagebuilder/README.md)
- **Development & Testing**: [mongoclient/README.md](mongoclient/README.md)