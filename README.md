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
| `gcc-asan` | Address | GCC | Recommended for daily dev. |
| `clang-asan` | Address | Clang | Alternative ASan build. |
| `clang-tsan` | Thread | Clang | Requires `docker run --security-opt seccomp=unconfined` to run tests. |
| `clang-msan` | Memory | Clang | **Experimental**. Fails at runtime due to ABI mismatch with system libraries. |

**Example (GCC ASan):**
```bash
cmake --preset gcc-asan
cmake --build --preset gcc-asan -j$(($(nproc)/2))
```

## Testing

Run unit tests using the matching test preset:

```bash
ctest --preset <preset-name>
```

**Example:**
```bash
ctest --preset gcc-asan
```

### Running Specific Tests
To run a specific test (e.g., `http2_server_unit_tests`) within a preset:
```bash
cd build/<preset-name>
ctest -R http2_server_unit_tests --output-on-failure
```

### Valgrind Verification
Valgrind targets are available in **Debug** builds (e.g., `gcc-debug` or `clang-debug`).

```bash
cmake --build --preset gcc-debug --target test_valgrind_all
```

## Fast Iteration (Pragmatic Workflow)

When working on a specific library (e.g., `logger`), you don't need to build the entire project.

1.  **Build Only the Component**:
    Target the specific test executable for the library you are modifying.
    ```bash
    # Build only the logger tests
    cmake --build --preset gcc-asan --target test_logger
    ```

2.  **Run Only the Component Tests**:
    Use `ctest -R` to run tests matching the library name.
    ```bash
    ctest --preset gcc-asan -R logger
    ```

This saves time by avoiding unnecessary compilation of unrelated components (like MongoDB or HTTP/2).

## 4. Verification (Valgrind)

We provide a unified command to run all Valgrind tools (MemCheck, Helgrind, Massif, Callgrind, Cachegrind).

### Run All Valgrind Checks
```bash
cd <build_dir>
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