# Astra

## 1. Development Environment

### Build Docker Image
```bash
docker build --network=host -t Astrabuilder:v11 -f tools/Dockerfile tools
```

### Run Container
```bash
docker run -it --name Astra -v $(pwd):/app/Astra Astrabuilder:v11 bash
```

### Run Container (with Sanitizer Support)
For running ThreadSanitizer (TSan) or AddressSanitizer (ASan) tests, use:
```bash
docker run -it --name Astra --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -v $(pwd):/app/Astra Astrabuilder:v11 bash
```
> **Note**: `--cap-add=SYS_PTRACE` enables process tracing for sanitizers. `--security-opt seccomp=unconfined` allows the `personality` syscall needed by TSan to disable ASLR.

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
cmake --build --preset <preset-name> -j2
```

### 2. Sanitizer Builds (Debug)

| Preset Name | Sanitizer | Compiler | Notes |
| :--- | :--- | :--- | :--- |
| `gcc-asan` | Address | GCC | **Recommended** for daily dev. |
| `gcc-tsan` | Thread | GCC | Thread safety via GCC. |
| `clang-asan` | Address | Clang | Alternative ASan build. |
| `clang-tsan` | Thread | Clang | **Note**: May fail in Docker due to `personality` syscall restrictions. |
| `clang-msan` | Memory | Clang | **Experimental**. Requires instrumented libc++. |

**Example (GCC ASan):**
```bash
cmake --preset gcc-asan
cmake --build --preset gcc-asan -j2
```

## Clean Builds

We use **Ninja** as the build system (faster than Make, better dependency tracking).

### Incremental Clean (Daily Development)
Removes build artifacts but preserves CMake cache. Fast reconfigure-free rebuilds.
```bash
ninja -C build/gcc-debug clean
```

### Full Clean (CMake Changes, Troubleshooting)
Nuclear option - deletes everything. Requires full reconfigure.
```bash
rm -rf build/gcc-debug

# Or clean all presets
rm -rf build/
```

**Clean → Build workflow:**
```bash
# Incremental (fast)
ninja -C build/gcc-debug clean
ninja -C build/gcc-debug -j2

# Full rebuild
rm -rf build/gcc-debug
cmake --preset gcc-debug
cmake --build --preset gcc-debug -j2
```

## Testing

We use **GoogleTest** for all unit tests.

### Run All Tests
```bash
# Using CTest (Recommended)
ctest --preset gcc-debug

# Or using Ninja directly
cd build/gcc-debug && ninja test
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

## Container Builds

Build and push container images to ghcr.io using CMake presets.

### Create GitHub Token

1. Go to **[github.com](https://github.com)** → Click profile picture → **Settings**
2. Scroll down left sidebar → **Developer settings**
3. **Personal access tokens** → **Tokens (classic)** → **Generate new token (classic)**
4. Configure:
   - **Note**: `ghcr-push` (or any name)
   - **Expiration**: 90 days (or your preference)
   - **Scopes**: ✅ `write:packages`, ✅ `read:packages`
5. Click **Generate token** and **copy immediately** (starts with `ghp_`)

> **Warning**: The token is only shown once. Save it securely!

### Build Container Image
```bash
# Configure first (if not already done)
cmake --preset clang-release

# Build the release binary
cmake --build --preset clang-release

# Build the container image
cmake --build --preset image-release
```

### Push to Registry
```bash
# Login to ghcr.io (set GITHUB_TOKEN first)
export GITHUB_TOKEN=ghp_xxxxx
echo "$GITHUB_TOKEN" | buildah login ghcr.io -u <username> --password-stdin

# Push the image
cmake --build --preset push-release
```

### Available Presets

| Preset | Description |
| :--- | :--- |
| `image-release` | Build release container (`uri-shortener:v1`) |
| `image-debug` | Build debug container (`uri-shortener:v1-debug`) |
| `push-release` | Push release image to ghcr.io |
| `push-debug` | Push debug image to ghcr.io |

### Verify Push

1. Go to [github.com](https://github.com) → Your Profile → **Packages** tab
2. Or direct: `https://github.com/YOUR_USERNAME?tab=packages`

> **Note**: Packages are private by default. To make public: Package settings → Danger Zone → Change visibility
