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
    && rm -rf /var/lib/apt/lists/*

# -----------------------------------------------------------------------------
# Build MongoDB C Driver 2.1.2 from source
# NOTE: This takes ~25 minutes during image build (one-time cost)
# The C++ driver will use this pre-installed version via find_package()
# -----------------------------------------------------------------------------
COPY scripts/build-mongo-driver.sh /tmp/
RUN chmod +x /tmp/build-mongo-driver.sh && \
    /tmp/build-mongo-driver.sh && \
    rm /tmp/build-mongo-driver.sh

WORKDIR /app

# MongoDB C driver pre-installed (2.1.2). C++ driver and nghttp2-asio built by CMake.

COPY scripts/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
