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

### Automated Build

You can use the `build.py` script to automate the entire process (build image, start container, compile & test):

```bash
./build.py --all
```

### Individual Steps

You can also run steps individually:

1.  **Build Image**: `./build.py --image`
2.  **Start Container**: `./build.py --start`
3.  **Compile & Test**: `./build.py --build`
4.  **Clean Up**: `./build.py --clean`

## Key Features

- **Version Control**: The MongoDB C++ driver version is managed via `mongoclient/mongodriver/CMakeLists.txt`.
- **Automatic Build**: The driver is automatically downloaded and built from source when you compile the project.
- **Dockerized**: Full development environment provided via Docker.

## Documentation

- **Docker Setup**: [imagebuilder/README.md](imagebuilder/README.md)
- **Development & Testing**: [mongoclient/README.md](mongoclient/README.md)