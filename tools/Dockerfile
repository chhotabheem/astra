# syntax = docker/dockerfile:1.4
FROM ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive

# -----------------------------------------------------------------------------
# System Dependencies
# -----------------------------------------------------------------------------
RUN apt-get update && apt-get install -y \
    #
    # Build Tools
    build-essential \
    cmake \
    git \
    ninja-build \
    pkg-config \
    #
    # C++ Compilers & Tools
    clang \
    libc++-dev \
    libc++abi-dev \
    #
    # C++ Libraries (Boost)
    libboost-chrono-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-url-dev \
    #
    # Load Testing Tools
    nghttp2-client \
    nginx \
    #
    # Security & Networking Libraries
    ca-certificates \
    libhiredis-dev \
    libnghttp2-dev \
    libsasl2-dev \
    libssl-dev \
    #
    # Debugging & Profiling
    gdb \
    lldb \
    valgrind \
    #
    # Utilities
    curl \
    dnsutils \
    gnupg \
    iproute2 \
    iputils-ping \
    net-tools \
    openssh-client \
    software-properties-common \
    tar \
    tree \
    vim \
    wget \
    protobuf-compiler \
    libprotobuf-dev \
    libgrpc++-dev \
    libgrpc-dev \
    protobuf-compiler-grpc \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# -----------------------------------------------------------------------------
# Build MongoDB C Driver 2.1.2 from source
# -----------------------------------------------------------------------------
COPY scripts/build-mongo-driver.sh /tmp/
RUN chmod +x /tmp/build-mongo-driver.sh && \
    /tmp/build-mongo-driver.sh && \
    rm /tmp/build-mongo-driver.sh

# -----------------------------------------------------------------------------
# Build OpenTelemetry C++ SDK v1.24.0 (C++20)
# NOTE: Pre-built so our C++17 code can link against it
# -----------------------------------------------------------------------------
COPY scripts/build-opentelemetry.sh /tmp/
RUN chmod +x /tmp/build-opentelemetry.sh && \
    /tmp/build-opentelemetry.sh && \
    rm /tmp/build-opentelemetry.sh

# -----------------------------------------------------------------------------
# Build Google Test (for unit testing)
# -----------------------------------------------------------------------------
COPY versions.env /tmp/
COPY scripts/build-googletest.sh /tmp/
RUN chmod +x /tmp/build-googletest.sh && \
    /tmp/build-googletest.sh && \
    rm /tmp/build-googletest.sh

# -----------------------------------------------------------------------------
# Build Google Benchmark (for performance testing)
# -----------------------------------------------------------------------------
COPY scripts/build-benchmark.sh /tmp/
RUN chmod +x /tmp/build-benchmark.sh && \
    /tmp/build-benchmark.sh && \
    rm /tmp/build-benchmark.sh

# NOTE: FuzzTest is downloaded via CMake FetchContent when ENABLE_FUZZTEST=ON
# No pre-installation needed

# -----------------------------------------------------------------------------
# Build MongoDB C++ Driver (uses system C driver)
# -----------------------------------------------------------------------------
COPY scripts/build-mongo-cxx-driver.sh /tmp/
RUN chmod +x /tmp/build-mongo-cxx-driver.sh && \
    /tmp/build-mongo-cxx-driver.sh && \
    rm /tmp/build-mongo-cxx-driver.sh

# -----------------------------------------------------------------------------
# Build redis-plus-plus (uses system hiredis)
# -----------------------------------------------------------------------------
COPY scripts/build-redis-plus-plus.sh /tmp/
RUN chmod +x /tmp/build-redis-plus-plus.sh && \
    /tmp/build-redis-plus-plus.sh && \
    rm /tmp/build-redis-plus-plus.sh

# -----------------------------------------------------------------------------
# Build nghttp2-asio with Boost 1.88 compatibility patch
# -----------------------------------------------------------------------------
COPY scripts/boost-1.88-compat.patch /tmp/
COPY scripts/build-nghttp2-asio.sh /tmp/
RUN chmod +x /tmp/build-nghttp2-asio.sh && \
    /tmp/build-nghttp2-asio.sh && \
    rm /tmp/build-nghttp2-asio.sh /tmp/boost-1.88-compat.patch

# Cleanup versions.env from /tmp
RUN rm -f /tmp/versions.env

WORKDIR /app

# Pre-installed: MongoDB C (2.1.2), OpenTelemetry C++ (v1.24.0),
#                GoogleTest, MongoDB C++, redis++, nghttp2-asio

COPY scripts/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
