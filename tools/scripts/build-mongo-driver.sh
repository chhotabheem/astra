#!/bin/bash
# Build and install MongoDB C Driver 2.1.2
# This script is called during Docker image build (one-time ~25 min)

set -e  # Exit on any error

echo "==> Downloading MongoDB C Driver 2.1.2..."
cd /tmp
git clone --depth 1 --branch 2.1.2 https://github.com/mongodb/mongo-c-driver.git

echo "==> Building MongoDB C Driver..."
cd mongo-c-driver
mkdir -p build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
    -DENABLE_TESTS=OFF \
    -DENABLE_EXAMPLES=OFF \
    -DENABLE_EXTRA_ALIGNMENT=OFF

make -j2

echo "==> Installing MongoDB C Driver to /usr/local..."
make install
ldconfig

echo "==> Cleaning up build artifacts..."
cd /
rm -rf /tmp/mongo-c-driver

echo "==> MongoDB C Driver 2.1.2 installed successfully!"
