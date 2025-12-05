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

WORKDIR /app

# Pre-installed: MongoDB C (2.1.2), OpenTelemetry C++ (v1.24.0)

COPY scripts/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
