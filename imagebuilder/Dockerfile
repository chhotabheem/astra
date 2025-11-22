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

RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r4.1.4/mongo-cxx-driver-r4.1.4.tar.gz && \
    tar -xzf mongo-cxx-driver-r4.1.4.tar.gz && \
    cd mongo-cxx-driver-r4.1.4 && \
    mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 && \
    cmake --build . && \
    cmake --build . --target install && \
    cd ../.. && \
    rm -rf mongo-cxx-driver-r4.1.4 mongo-cxx-driver-r4.1.4.tar.gz


