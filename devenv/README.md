# Docker Environment Setup

## Overview

This directory contains the Docker configuration and build automation for the Astra project. The Docker image includes all necessary dependencies for building and running the C++ services.

## Quick Start

### Build the Docker Image

```bash
./imagebuilder/build_container.py --build
```

This will build the `astrabuilder:nghttp2` image with:
- Ubuntu 25.10 base
- Build tools (CMake, GCC, Ninja)
- Boost libraries (system, thread, chrono)
- nghttp2 library
- OpenSSL, SASL
- All required development headers

**Build time:** ~2-3 minutes

### Start the Container

```bash
./imagebuilder/build_container.py --start
```

This creates and starts a container named `astra` with your project directory mounted at `/app/astra`.

### Build and Start (One Command)

```bash
./imagebuilder/build_container.py --up
```

### Stop the Container

```bash
./imagebuilder/build_container.py --stop
```

## Advanced Usage

### Custom Image Tag

```bash
./imagebuilder/build_container.py --build --image astrabuilder:dev
```

### Custom Configuration

```bash
./imagebuilder/build_container.py --build --config my-config.json
```

### Network Configuration

The build script uses `--network=host` by default to ensure CMake FetchContent can download dependencies from GitHub. This is configured in `config.json`.

## Manual Docker Commands

If you need to run Docker commands manually:

### Build Image

```bash
cd imagebuilder
docker build --network=host -t astrabuilder:nghttp2 -f Dockerfile ../
```

### Run Container

```bash
docker run -d --name astra --network host \
  -v $(pwd):/app/astra \
  astrabuilder:nghttp2 tail -f /dev/null
```

### Enter Container

```bash
docker exec -it astra /bin/bash
```

## Configuration

The build script uses `config.json` for default settings:

```json
{
    "image_name": "astrabuilder:nghttp2",
    "container_name": "astra",
    "container_workdir": "/app/astra",
    "network_mode": "host",
    "build_network": "host",
    "log_level": "INFO",
    "dockerfile_path": "."
}
```

## Dockerfile Details

### System Dependencies

The Dockerfile installs:
- **Build tools:** cmake, ninja-build, pkg-config, git
- **Boost:** libboost-system-dev, libboost-thread-dev, libboost-chrono-dev
- **HTTP/2:** libnghttp2-dev
- **Security:** libssl-dev, libsasl2-dev
- **Utilities:** curl, tar, tree, ca-certificates

### CMake FetchContent Dependencies

The following are downloaded during project build (not baked into image):
- **nghttp2-asio** v1.62.1 (HTTP/2 server library)
- **MongoDB C++ Driver** r4.1.4
- **spdlog** v1.12.0 (logging library)

## Troubleshooting

### DNS Resolution Issues

If CMake FetchContent fails to download dependencies:
- Ensure `--network=host` is used for builds
- Check GitHub is accessible from your network
- Verify DNS resolution works in container

### Build Failures

If the Docker image fails to build:
- Check available disk space
- Ensure Docker daemon is running
- Verify network connectivity

### Container Won't Start

If the container fails to start:
- Check if port conflicts exist
- Verify the image was built successfully
- Check Docker logs: `docker logs astra`

## Project Structure

```
imagebuilder/
├── Dockerfile              # Docker image definition
├── build_container.py      # Build automation script (entry point)
├── config.json             # Default configuration
├── astra_builder/          # Python package
│   ├── __init__.py
│   ├── cli.py              # Command-line interface
│   ├── docker_manager.py   # Docker operations
│   ├── config.py           # Configuration management
│   └── logger.py           # Logging setup
└── README.md               # This file
```

## Next Steps

After the container is running, see the main [README.md](../README.md) for instructions on building and testing the project modules.
