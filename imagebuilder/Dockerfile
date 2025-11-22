FROM ubuntu:25.10

RUN apt update && apt install -y \
    build-essential \
    cmake \
    pkg-config \
    curl \
    tar \
    git \
    tree \
    libssl-dev \
    libsasl2-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
ENV LD_LIBRARY_PATH=/usr/local/lib/:${LD_LIBRARY_PATH}

# Attempt to set DNS for internet access
RUN echo "nameserver 8.8.8.8" > /etc/resolv.conf || echo "Could not modify /etc/resolv.conf, proceeding..."

# MongoDB driver will be built by CMake during the project build



