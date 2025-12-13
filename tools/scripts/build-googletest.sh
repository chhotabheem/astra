#!/bin/bash
# Build and install Google Test
set -e

source /tmp/versions.env

VERSION="${GOOGLETEST_VERSION}"
BUILD_DIR="/tmp/googletest"

echo "==> Downloading Google Test ${VERSION}..."
curl -fsSL "https://github.com/google/googletest/archive/refs/tags/v${VERSION}.tar.gz" | tar xz -C /tmp
mv "/tmp/googletest-${VERSION}" "${BUILD_DIR}"

echo "==> Building Google Test..."
cd "${BUILD_DIR}"
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_SHARED_LIBS=OFF

cmake --build build -j2

echo "==> Installing Google Test..."
cmake --install build
ldconfig

echo "==> Cleaning up..."
rm -rf "${BUILD_DIR}"

echo "==> Google Test ${VERSION} installed successfully!"
