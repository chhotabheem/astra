#!/bin/bash
# Build and install Google Benchmark
set -e

source /tmp/versions.env

VERSION="${GOOGLE_BENCHMARK_VERSION}"
BUILD_DIR="/tmp/benchmark"

echo "==> Downloading Google Benchmark ${VERSION}..."
curl -fsSL "https://github.com/google/benchmark/archive/refs/tags/${VERSION}.tar.gz" | tar xz -C /tmp
mv "/tmp/benchmark-${VERSION#v}" "${BUILD_DIR}"

echo "==> Building Google Benchmark..."
cd "${BUILD_DIR}"
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBENCHMARK_ENABLE_TESTING=OFF \
    -DBENCHMARK_ENABLE_GTEST_TESTS=OFF

cmake --build build -j2

echo "==> Installing Google Benchmark..."
cmake --install build
ldconfig

echo "==> Cleaning up..."
rm -rf "${BUILD_DIR}"

echo "==> Google Benchmark ${VERSION} installed successfully!"
