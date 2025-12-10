#!/bin/bash
set -e

# Fix DNS by overwriting /etc/resolv.conf
# This is necessary because Docker might inherit a bad search domain configuration
echo "nameserver 8.8.8.8" > /etc/resolv.conf
echo "nameserver 8.8.4.4" >> /etc/resolv.conf

# Fix file descriptor limit for Valgrind compatibility in Docker
# Valgrind fails with "Private file creation failed" when ulimit is too high
ulimit -n 1024 2>/dev/null || true

# Execute the command passed to docker run
exec "$@"
