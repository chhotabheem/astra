#!/bin/bash
# Build and install redis-plus-plus (uses pre-installed hiredis)
set -e

source /tmp/versions.env

VERSION="${REDIS_PLUS_PLUS_VERSION}"
BUILD_DIR="/tmp/redis-plus-plus"

echo "==> Downloading redis-plus-plus ${VERSION}..."
curl -fsSL "https://github.com/sewenew/redis-plus-plus/archive/refs/tags/${VERSION}.tar.gz" | tar xz -C /tmp
mv "/tmp/redis-plus-plus-${VERSION}" "${BUILD_DIR}"

echo "==> Building redis-plus-plus..."
cd "${BUILD_DIR}"
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_SHARED_LIBS=OFF \
    -DREDIS_PLUS_PLUS_BUILD_TEST=OFF \
    -DREDIS_PLUS_PLUS_BUILD_STATIC=ON

cmake --build build -j2

echo "==> Installing redis-plus-plus..."
cmake --install build
ldconfig

echo "==> Cleaning up..."
rm -rf "${BUILD_DIR}"

echo "==> redis-plus-plus ${VERSION} installed successfully!"
