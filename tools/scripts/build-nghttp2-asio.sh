#!/bin/bash
# Build and install nghttp2-asio with Boost 1.88 compatibility patch
set -e

source /tmp/versions.env

VERSION="${NGHTTP2_ASIO_VERSION}"
BUILD_DIR="/tmp/nghttp2-asio"
PATCH_FILE="/tmp/boost-1.88-compat.patch"

echo "==> Downloading nghttp2-asio (main branch)..."
curl -fsSL "https://github.com/nghttp2/nghttp2-asio/archive/refs/heads/main.tar.gz" | tar xz -C /tmp
mv /tmp/nghttp2-asio-main "${BUILD_DIR}"

echo "==> Applying Boost 1.88 compatibility patch..."
cd "${BUILD_DIR}"
patch -p0 -N -f < "${PATCH_FILE}"

echo "==> Building nghttp2-asio..."
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_SHARED_LIBS=OFF \
    -DENABLE_APP=OFF \
    -DENABLE_HPACK_TOOLS=OFF \
    -DENABLE_EXAMPLES=OFF

cmake --build build -j2

echo "==> Installing nghttp2-asio..."
cmake --install build
ldconfig

echo "==> Cleaning up..."
rm -rf "${BUILD_DIR}"

echo "==> nghttp2-asio ${VERSION} installed successfully!"
