# Docker Environment Setup

This directory contains the Docker configuration for the Astra project.

## Docker Setup

### 1. Build the Docker Image
The `Dockerfile` sets up an Ubuntu environment with all necessary dependencies (CMake, OpenSSL, SASL, git, tree, etc.).
**Note:** The MongoDB C++ driver is NOT baked into the image. It is downloaded and built automatically by CMake when you compile the project.

```bash
# Run from the imagebuilder directory
cd imagebuilder
docker build -t astrabuilder .
```

**Note:** The Dockerfile includes a DNS fix (`nameserver 8.8.8.8`) to ensure internet connectivity during the build process.

### 2. Create the Persistent Container
We use a persistent container named `prayag` to keep the environment running.

```bash
# Remove any existing container named prayag
docker rm -f prayag

# Run the container in detached mode (-d)
# Note: Run this from the project root directory (parent of imagebuilder)
# -v $(pwd):/app/astra : Mounts your project root directory to /app/astra inside the container
# tail -f /dev/null    : Keeps the container running indefinitely
cd ..
docker run -d --name prayag -v $(pwd):/app/astra astrabuilder tail -f /dev/null
```

**Note:** It is recommended to use the `manage_env.py` script in this directory to manage this process automatically:

```bash
./manage_env.py --up
```

## Next Steps

Once the container is running, refer to [../mongoclient/README.md](../mongoclient/README.md) for instructions on compiling and running the application.
