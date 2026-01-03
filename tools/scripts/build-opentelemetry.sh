#!/bin/bash
# Build OpenTelemetry C++ SDK v1.24.0 with C++20
set -e

OTEL_VERSION="v1.24.0"
BUILD_DIR="/tmp/otel-build"

echo "=== Building OpenTelemetry C++ SDK ${OTEL_VERSION} ==="

git clone --depth 1 --branch ${OTEL_VERSION} \
    https://github.com/open-telemetry/opentelemetry-cpp.git ${BUILD_DIR}

cd ${BUILD_DIR}

cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_TESTING=OFF \
    -DWITH_BENCHMARK=OFF \
    -DWITH_EXAMPLES=OFF \
    -DWITH_OTLP_GRPC=OFF \
    -DWITH_OTLP_HTTP=OFF \
    -DWITH_PROMETHEUS=OFF \
    -DWITH_JAEGER=OFF \
    -DBUILD_SHARED_LIBS=OFF

cmake --build build -j2
cmake --install build

rm -rf ${BUILD_DIR}
echo "=== Done ==="
