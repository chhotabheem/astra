# Ninja Migration - Next Steps

## Changes Complete ✅

1. ✅ Added `ninja-build` to [Dockerfile](file:///home/siddu/astra/devenv/Dockerfile)
2. ✅ Changed all presets to Ninja in [CMakePresets.json](file:///home/siddu/astra/CMakePresets.json)
3. ✅ Removed `cmake/clean.cmake` (over-engineered solution)
4. ✅ Updated [README.md](file:///home/siddu/astra/README.md) with new clean workflows

## Required: Rebuild Docker Image

The current container doesn't have Ninja. You must rebuild the Docker image:

```bash
docker build --network=host -t astrabuilder:v6 -f devenv/Dockerfile devenv
```

**Note:** This will take ~25 minutes (one-time cost due to MongoDB C driver build).

## Verification Steps

After rebuilding the image and recreating the container:

### 1. Verify Ninja Installation
```bash
docker exec -it astra ninja --version
```

### 2. Clean Old Builds
```bash
docker exec -it astra bash -c "cd /app/astra && rm -rf build/"
```

### 3. Test Build with Ninja
```bash
docker exec -it astra bash -c "cd /app/astra && cmake --preset gcc-debug"
docker exec -it astra bash -c "cd /app/astra && cmake --build --preset gcc-debug -j2"
```

### 4. Test Clean Commands
```bash
# Incremental clean
docker exec -it astra bash -c "cd /app/astra && ninja -C build/gcc-debug clean"

# Verify it cleaned
docker exec -it astra bash -c "cd /app/astra && ls build/gcc-debug/"
```

### 5. Run Tests
```bash
docker exec -it astra bash -c "cd /app/astra && ctest --preset gcc-debug"
```

## Pending Verification (To Do Later)

### 1. Verify Thread Safety (Helgrind)
Run Helgrind to check for race conditions and locking errors.
```bash
docker exec -it astra bash -c "cd /app/astra/build/gcc-debug && ninja test_helgrind"
```

### 2. Verify ThreadSanitizer (Clang TSan)
Run the ThreadSanitizer build to catch data races.
```bash
docker exec -it astra bash -c "cd /app/astra && cmake --preset clang-tsan"
docker exec -it astra bash -c "cd /app/astra && cmake --build --preset clang-tsan -j2"
docker exec -it astra bash -c "cd /app/astra && ctest --preset clang-tsan"
```

## New Clean Workflow Reference

**Daily development (incremental):**
```bash
ninja -C build/gcc-debug clean
ninja -C build/gcc-debug -j2
```

**After CMake changes (full rebuild):**
```bash
rm -rf build/gcc-debug
cmake --preset gcc-debug
cmake --build --preset gcc-debug -j2
```
