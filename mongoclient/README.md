# MongoDB C++ Client Development

This directory contains the source code for the MongoDB C++ client library and test application.

## Compilation and Testing

The project uses CMake for building. The MongoDB C++ driver dependencies are automatically fetched and built from source during the build process.

### Prerequisites

Ensure you are running inside the Docker container provided by the `imagebuilder` directory.

```bash
docker exec -it prayag /bin/bash
```

### Build Instructions

Inside the container:

1.  **Navigate to the project root:**
    ```bash
    cd /app/astra
    ```

2.  **Create a build directory:**
    ```bash
    mkdir -p build && cd build
    ```

3.  **Configure with CMake:**
    ```bash
    cmake ..
    ```
    *This step will automatically download and build the MongoDB C++ driver (and C driver) if they are not already present.*

4.  **Compile:**
    ```bash
    cmake --build .
    ```

### Running Tests

Run the tests using CTest:

```bash
ctest --output-on-failure
```

### Running the Application

Run the main application:

```bash
./mongo_app
```

**Note:** The application attempts to connect to a MongoDB server at `172.17.0.3:27017`. Ensure a MongoDB instance is reachable at that address.

### One-liner Command

You can compile and test everything from your host machine with a single command:

```bash
docker exec -it prayag /bin/bash -c "cd /app/astra/mongoclient && mkdir -p build && cd build && cmake .. && cmake --build . && ctest --output-on-failure && ./mongo_app"
```

## Project Structure

- `CMakeLists.txt`: Main build configuration.
- `mongodriver/`: Contains CMake configuration for fetching and building the MongoDB driver.
- `IMongoClient.h`: Interface definition.
- `MongoClient.h/cpp`: Implementation.
- `main.cpp`: Main application entry point.
- `tests/`: Unit tests.
