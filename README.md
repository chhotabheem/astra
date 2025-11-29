# Astra

## 1. Development Environment

### Build Docker Image
```bash
docker build --network=host -t astrabuilder:v1 -f devenv/Dockerfile devenv
```

### Run Container
```bash
docker run -it --name astra -v $(pwd):/app/astra astrabuilder:v1 bash
```

## 2. Building

### Release Build (Default)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Debug Build
```bash
cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug -j$(nproc)
```

### Clang Build
```bash
cmake -S . -B build_clang -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build_clang -j$(nproc)
```

## 3. Testing

### Run All Unit Tests
```bash
cd build
ctest --output-on-failure
```

### Run Tests for a Specific Library
To run tests for a specific library (e.g., `http2server`), use the `-R` (regex) flag with the directory name (lowercase):

```bash
cd build
# Run all tests matching "http2"
ctest -R http2 --output-on-failure

# Run all tests matching "redis"
ctest -R redis --output-on-failure
```

## 4. Verification (Valgrind)

We provide a unified command to run all Valgrind tools (MemCheck, Helgrind, Massif, Callgrind, Cachegrind).

### Run All Valgrind Checks
```bash
cd build
make test_valgrind_all
```

### Run Specific Checks
```bash
make test_memcheck   # Memory Leaks
make test_helgrind   # Thread Safety
make test_massif     # Heap Profiling
make test_callgrind  # Call Graph Profiling
make test_cachegrind # Cache Profiling
```