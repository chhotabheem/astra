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

# 1. Replace boost::asio::io_service with boost::asio::io_context
echo "Replacing io_service with io_context..."
find "$SOURCE_DIR" -type f \( -name "*.h" -o -name "*.cc" \) -exec sed -i 's/boost::asio::io_service/boost::asio::io_context/g' {} +

# 2. Replace forward declarations
echo "Replacing forward declarations..."
find "$SOURCE_DIR" -type f \( -name "*.h" -o -name "*.cc" \) -exec sed -i 's/class io_service;/class io_context;/g' {} +

# 2.5 Replace io_service_.post() with boost::asio::post(io_service_, ...)
echo "Replacing io_service_.post() with boost::asio::post()..."
find "$SOURCE_DIR" -type f \( -name "*.h" -o -name "*.cc" \) -exec sed -i 's/io_service_.post(/boost::asio::post(io_service_, /g' {} +
find "$SOURCE_DIR" -type f \( -name "*.h" -o -name "*.cc" \) -exec sed -i 's/io_service_.dispatch(/boost::asio::dispatch(io_service_, /g' {} +

# 2.6 Replace tcp::resolver::iterator with tcp::resolver::results_type::iterator
echo "Replacing tcp::resolver::iterator..."
find "$SOURCE_DIR" -type f \( -name "*.h" -o -name "*.cc" \) -exec sed -i 's/tcp::resolver::iterator/tcp::resolver::results_type::iterator/g' {} +
find "$SOURCE_DIR" -type f \( -name "*.h" -o -name "*.cc" \) -exec sed -i 's/boost::asio::ip::resolver::iterator/boost::asio::ip::tcp::resolver::results_type::iterator/g' {} +

# 2.7 Fix async_resolve in asio_client_session_impl.cc
echo "Fixing async_resolve in asio_client_session_impl.cc..."
CLIENT_IMPL="$SOURCE_DIR/lib/asio_client_session_impl.cc"
if [ -f "$CLIENT_IMPL" ]; then
    # Replace query-based resolve with direct resolve
    sed -i 's/resolver_.async_resolve({host, service},/resolver_.async_resolve(host, service,/g' "$CLIENT_IMPL"
    
    # Replace callback signature to take results_type instead of iterator
    # The code is split across lines:
    # [self](const boost::system::error_code &ec,
    #        tcp::resolver::results_type::iterator endpoint_it) {
    # We want to change the second line to: tcp::resolver::results_type results) {
    # WARNING: This pattern also matches "void session_impl::connected(tcp::resolver::results_type::iterator endpoint_it) {"
    # So we must fix "connected" back if it gets changed.
    
    sed -i 's/tcp::resolver::results_type::iterator endpoint_it) {/tcp::resolver::results_type results) {/g' "$CLIENT_IMPL"
    
    # Restore session_impl::connected signature if it was changed
    sed -i 's/void session_impl::connected(tcp::resolver::results_type results) {/void session_impl::connected(tcp::resolver::results_type::iterator endpoint_it) {/g' "$CLIENT_IMPL"
    
    # Pass results.begin() to start_connect
    # This was already working but good to keep
    sed -i 's/self->start_connect(endpoint_it);/self->start_connect(results.begin());/g' "$CLIENT_IMPL"
else
    echo "Warning: $CLIENT_IMPL not found"
fi

# Fix header declaration in asio_client_session_impl.h
# We don't need to change this if we keep it as iterator.
# The global replacement handled it.

# 2.8 Fix async_connect in asio_client_session_tcp_impl.cc and asio_client_session_tls_impl.cc
echo "Fixing async_connect in client session implementations..."

# For TLS impl
TLS_IMPL="$SOURCE_DIR/lib/asio_client_session_tls_impl.cc"
if [ -f "$TLS_IMPL" ]; then
    # The call is split:
    # boost::asio::async_connect(
    #     socket(), endpoint_it,
    #     ...\n    # We want: socket(), endpoint_it, tcp::resolver::results_type::iterator(),
    # Use a more specific pattern to avoid double-replacement
    # Match only lines that don't already have results_type::iterator() after endpoint_it
    
    sed -i '/tcp::resolver::results_type::iterator(),.*tcp::resolver::results_type::iterator()/! s/socket(), endpoint_it,$/socket(), endpoint_it, tcp::resolver::results_type::iterator(),/g' "$TLS_IMPL"
    
    # Fix deprecated rfc2818_verification
    sed -i 's/boost::asio::ssl::rfc2818_verification/boost::asio::ssl::host_name_verification/g' "$TLS_IMPL"
fi

# For TCP impl
TCP_IMPL="$SOURCE_DIR/lib/asio_client_session_tcp_impl.cc"
if [ -f "$TCP_IMPL" ]; then
    # Similarly for TCP - avoid double-replacement
    sed -i '/tcp::resolver::results_type::iterator(),.*tcp::resolver::results_type::iterator()/! s/socket_, endpoint_it,$/socket_, endpoint_it, tcp::resolver::results_type::iterator(),/g' "$TCP_IMPL"
fi

# 3. Replace io_service::work with executor_work_guard
echo "Replacing io_service::work..."

# In asio_io_service_pool.h
POOL_H="$SOURCE_DIR/lib/asio_io_service_pool.h"
if [ -f "$POOL_H" ]; then
    echo "Patching $POOL_H"
    sed -i 's/std::vector<std::shared_ptr<boost::asio::io_context::work>> work_;/std::vector<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_;/g' "$POOL_H"
else
    echo "Warning: $POOL_H not found"
fi

# In asio_io_service_pool.cc
POOL_CC="$SOURCE_DIR/lib/asio_io_service_pool.cc"
if [ -f "$POOL_CC" ]; then
    echo "Patching $POOL_CC"
    # Replace make_shared work with make_work_guard
    sed -i 's/auto work = std::make_shared<boost::asio::io_context::work>(\*io_service);/auto work = boost::asio::make_work_guard(*io_service);/g' "$POOL_CC"
    
    # Replace push_back(work) with push_back(std::move(work))
    sed -i 's/work_.push_back(work);/work_.push_back(std::move(work));/g' "$POOL_CC"
    
    # Remove work_.clear() in stop() as guards destruct automatically
    sed -i 's/work_.clear();/\/\/ work_.clear();/g' "$POOL_CC"
else
    echo "Warning: $POOL_CC not found"
fi

# 4. Copy fixed asio_server.cc (handles resolver iterator and max_connections deprecation)
PATCH_DIR=$(dirname "$0")/patches
echo "Looking for patches in: $PATCH_DIR"
ls -l "$PATCH_DIR"

if [ -f "$PATCH_DIR/asio_server.cc" ]; then
    echo "Copying fixed asio_server.cc..."
    cp "$PATCH_DIR/asio_server.cc" "$SOURCE_DIR/lib/asio_server.cc"
else
    echo "Warning: Fixed asio_server.cc not found at $PATCH_DIR/asio_server.cc"
fi

echo "Patching complete."
echo "========================================================"
