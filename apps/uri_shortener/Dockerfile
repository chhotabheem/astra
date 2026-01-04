# syntax=docker/dockerfile:1.4
# =============================================================================
# URI Shortener - Runtime Container Image (Kubernetes)
# =============================================================================
# Build with Buildah (from astra root directory, INSIDE the dev container):
#   buildah bud -f apps/uri_shortener/Dockerfile --target release -t uri-shortener:v1 .
#   buildah bud -f apps/uri_shortener/Dockerfile --target debug -t uri-shortener:v1-debug .
#
# Prerequisites:
#   1. Run inside the dev container (astrabuilder)
#   2. Compile binary: cmake --preset clang-release && cmake --build build/clang-release
#
# Note: Libraries are copied from /usr/local/lib which exists in the dev container.
#       The build context includes the lib directory via the libs/ symlink created below.
# =============================================================================

# -----------------------------------------------------------------------------
# Stage: base - Minimal runtime with dependencies
# -----------------------------------------------------------------------------
FROM ubuntu:25.10 AS base

ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime libraries (no -dev, no tools)
RUN apt-get update && apt-get install -y --no-install-recommends \
    libprotobuf32t64 \
    libboost-chrono1.88.0 \
    libboost-system1.88.0 \
    libboost-thread1.88.0 \
    libboost-url1.88.0 \
    libssl3t64 \
    libnghttp2-14 \
    zlib1g \
    libzstd1 \
    libhiredis1.1.0 \
    libgrpc++1.51t64 \
    libgrpc29t64 \
    libcurl4t64 \
    libsasl2-2 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Copy custom-built libraries from build context
# These are copied from /usr/local/lib in the dev container
COPY libs/ /usr/local/lib/

RUN ldconfig

# Non-root user for K8s security policies
# Use existing ubuntu group (GID 1000) or create new one
RUN groupadd --gid 1000 appgroup 2>/dev/null || true && \
    useradd --uid 1000 --gid 1000 --shell /sbin/nologin --no-create-home appuser 2>/dev/null || true

WORKDIR /app
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# -----------------------------------------------------------------------------
# Stage: release - Production image
# -----------------------------------------------------------------------------
FROM base AS release

COPY --chown=1000:1000 build/clang-release/bin/uri_shortener /app/uri_shortener

USER 1000:1000

ENTRYPOINT ["/app/uri_shortener"]
CMD ["--config", "/app/config/config.json"]

# -----------------------------------------------------------------------------
# Stage: debug - Development/troubleshooting image
# -----------------------------------------------------------------------------
FROM base AS debug

RUN apt-get update && apt-get install -y --no-install-recommends \
    gdb strace \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

COPY --chown=1000:1000 build/clang-debug/bin/uri_shortener /app/uri_shortener

ENTRYPOINT ["/app/uri_shortener"]
CMD ["--config", "/app/config/config.json"]
