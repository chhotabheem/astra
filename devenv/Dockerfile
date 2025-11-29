# syntax = docker/dockerfile:1.4
FROM ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive


# -----------------------------------------------------------------------------
# 1. System Dependencies
# -----------------------------------------------------------------------------
# Install build tools and libraries required for C++ development and nghttp2-asio
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    curl \
    tar \
    git \
    tree \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-chrono-dev \
    libssl-dev \
    libsasl2-dev \
    libnghttp2-dev \
    libhiredis-dev \
    ca-certificates \
    vim \
    iputils-ping \
    net-tools \
    gdb \
    valgrind \
    clang \
    libc++-dev \
    libc++abi-dev \
    lldb \
    wget \
    openssh-client \
    dnsutils \
    iproute2 \
    gnupg \
    software-properties-common && \
    rm -rf /var/lib/apt/lists/*



WORKDIR /app

# MongoDB driver and nghttp2-asio will be built by CMake during the project build
