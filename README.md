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

## Build Instructions

We use **CMake Presets** to simplify building with different compilers and configurations. All builds are output to the `build/` directory.

### 1. Standard Builds

| Preset Name | Compiler | Build Type | Output Directory |
| :--- | :--- | :--- | :--- |
| `gcc-release` | GCC | Release | `build/gcc-release` |
| `gcc-debug` | GCC | Debug | `build/gcc-debug` |
| `clang-release` | Clang | Release | `build/clang-release` |
| `clang-debug` | Clang | Debug | `build/clang-debug` |

**Command:**
```bash
# Configure
cmake --preset <preset-name>

# Build (Recommended: Use half of available cores to avoid freezing)
cmake --build --preset <preset-name> -j$(($(nproc)/2))
```

### 2. Sanitizer Builds (Debug)

| Preset Name | Sanitizer | Compiler | Notes |
| :--- | :--- | :--- | :--- |
| `gcc-asan` | Address | GCC | **Recommended** for daily dev. |
| `clang-asan` | Address | Clang | Alternative ASan build. |
| `clang-tsan` | Thread | Clang | **Note**: May fail in Docker due to `personality` syscall restrictions. |
| `clang-msan` | Memory | Clang | **Experimental**. Requires instrumented libc++. |

**Example (GCC ASan):**
```bash
cmake --preset gcc-asan
cmake --build --preset gcc-asan -j$(($(nproc)/2))
```

## Testing

We use **GoogleTest** for all unit tests.

### Run All Tests
```bash
# Using CTest (Recommended)
ctest --preset gcc-debug

# Or using Make directly
cd build/gcc-debug && make test
```

### Run Specific Component Tests
You can run tests for a specific component (e.g., `router`, `http1.1server`) by targeting its GTest executable.

```bash
# Build and run just the router tests
cmake --build --preset gcc-debug --target router_gtest
./build/gcc-debug/router/tests/router_gtest
```

### Run Specific Test Cases
Use `--gtest_filter` to run specific test cases.

```bash
# Run only the ExactMatch test in Router
./build/gcc-debug/router/tests/router_gtest --gtest_filter=RouterTest.ExactMatch
```

## Verification (Valgrind)

We provide a unified target to run all Valgrind tools (MemCheck, Helgrind, Massif, Callgrind, Cachegrind).
**Note**: Valgrind targets are only available in **Debug** builds (e.g., `gcc-debug`).

### Run All Valgrind Checks
```bash
cmake --build --preset gcc-debug --target test_valgrind_all
```

### Run Specific Checks
```bash
cmake --build --preset gcc-debug --target test_memcheck   # Memory Leaks
cmake --build --preset gcc-debug --target test_helgrind   # Thread Safety
cmake --build --preset gcc-debug --target test_massif     # Heap Profiling
```