# MongoDB C++ Driver Test Environment

This project contains a Dockerized environment for developing and testing the MongoDB C++ client library.

## Docker Setup

### 1. Build the Docker Image
The `Dockerfile` (located in the `imagebuilder` directory) sets up an Ubuntu environment with all necessary dependencies (CMake, OpenSSL, SASL, etc.) and compiles the `mongo-cxx-driver` (v4.1.4) from source.

```bash
# Run from the imagebuilder directory
cd imagebuilder
docker build -t mongo-test .
```

**Note:** The Dockerfile includes a DNS fix (`nameserver 8.8.8.8`) to ensure internet connectivity during the build process.

### 2. Create the Persistent Container
We use a persistent container named `prayag` to keep the environment running. This allows you to log in and run tests repeatedly without rebuilding or restarting.

```bash
# Remove any existing container named prayag
docker rm -f prayag

# Run the container in detached mode (-d)
# Note: Run this from the project root directory (parent of imagebuilder)
# -v $(pwd):/app/astra : Mounts your project root directory to /app/astra inside the container
# tail -f /dev/null    : Keeps the container running indefinitely
cd ..
docker run -d --name prayag -v $(pwd):/app/astra mongo-test tail -f /dev/null
```

## Compiling and Running Tests

Since your code is mounted into the container, you must compile it **inside** the container.

### 1. Access the Container
Log in to the running `prayag` container:
```bash
docker exec -it prayag /bin/bash
```

### 2. Compile the Code
Inside the container, navigate to the source directory and build:

```bash
# Go to the source directory (mounted volume)
cd /app/astra/mongoclient

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Compile
cmake --build .
```

### 3. Run Tests
After compiling, run the tests using CTest:

```bash
ctest --output-on-failure
```

### 4. Run the Main Application
To run the main application:

```bash
./mongo_app
```

**Note:** The application attempts to connect to a MongoDB server at `172.17.0.3:27017`. Ensure a MongoDB instance is reachable at that address, or update the connection string in `main.cpp`.

### One-liner Command
You can also do everything (compile & test) from your host machine in one command:

```bash
docker exec -it prayag /bin/bash -c "cd /app/astra/mongoclient && mkdir -p build && cd build && cmake .. && cmake --build . && ctest --output-on-failure && ./mongo_app"
```

## Project Structure inside Container
- `/app/mongo-cxx-driver-r4.1.4`: The installed MongoDB C++ driver source (built during image creation).
- `/app/astra`: Your project root directory (mounted via volume).
- `/app/astra/mongoclient`: Your MongoDB client code directory.
- `/app/astra/imagebuilder`: Contains the Dockerfile and this README.
