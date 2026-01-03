#!/bin/bash
# Build and install MongoDB C++ Driver (uses pre-installed C driver)
set -e

source /tmp/versions.env

VERSION="${MONGO_CXX_DRIVER_VERSION}"
BUILD_DIR="/tmp/mongo-cxx-driver"

echo "==> Downloading MongoDB C++ Driver ${VERSION}..."
curl -fsSL "https://github.com/mongodb/mongo-cxx-driver/releases/download/${VERSION}/mongo-cxx-driver-${VERSION}.tar.gz" | tar xz -C /tmp
mv "/tmp/mongo-cxx-driver-${VERSION}" "${BUILD_DIR}"

echo "==> Building MongoDB C++ Driver..."
cd "${BUILD_DIR}"
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_SHARED_LIBS=OFF \
    -DENABLE_TESTS=OFF \
    -DENABLE_EXAMPLES=OFF \
    -DENABLE_UNINSTALL=OFF

cmake --build build -j2

echo "==> Installing MongoDB C++ Driver..."
cmake --install build
ldconfig

echo "==> Cleaning up..."
rm -rf "${BUILD_DIR}"

echo "==> MongoDB C++ Driver ${VERSION} installed successfully!"
