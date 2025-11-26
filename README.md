# Astra

## Building the Project

### Build Everything
```bash
cd /app/astra
cmake -B build -S .
cmake --build build
```

### Build Specific Target
```bash
cd /app/astra
cmake --build build --target logger
cmake --build build --target jsonformatter
cmake --build build --target httpserver
cmake --build build --target mongoclient
cmake --build build --target server
```

## Running Tests

### All Tests
```bash
cd /app/astra/build
ctest --output-on-failure
```

### Specific Module Tests
```bash
cd /app/astra/build/logger
ctest --output-on-failure

cd /app/astra/build/jsonformatter
ctest --output-on-failure
```

## Docker Build & Test (Host Machine)

Run these commands from your host machine to build and test inside the container:

### 1. Clean Build Directory
```bash
docker exec -w /app/astra astra rm -rf build
```

### 2. Configure CMake (Release)
```bash
docker exec -w /app/astra astra cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

### 3. Build All Targets
```bash
docker exec -w /app/astra astra cmake --build build -j4
```

### 4. Run All Tests
```bash
docker exec -w /app/astra/build astra ctest --output-on-failure
```