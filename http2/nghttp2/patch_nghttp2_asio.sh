#!/bin/bash

SOURCE_DIR=$1

if [ -z "$SOURCE_DIR" ]; then
    echo "Usage: $0 <source_directory>"
    exit 1
fi

echo "========================================================"
echo "PATCHING nghttp2-asio in: $SOURCE_DIR"
echo "========================================================"

# Check if directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Source directory does not exist: $SOURCE_DIR"
    exit 1
fi

# Apply the patch file
PATCH_FILE=$(dirname "$0")/boost-1.88-compat.patch

if [ ! -f "$PATCH_FILE" ]; then
    echo "Error: Patch file not found: $PATCH_FILE"
    exit 1
fi

echo "Applying patch: $PATCH_FILE"
cd "$SOURCE_DIR" || exit 1

# Try to apply the patch
if patch -p0 -N -f < "$PATCH_FILE"; then
    echo "Patch applied successfully"
else
    echo "Error: Failed to apply patch"
    exit 1
fi

echo "Patching complete."
echo "========================================================"
