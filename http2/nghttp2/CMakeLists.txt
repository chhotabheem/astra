cmake_minimum_required(VERSION 3.25)

include(FetchContent)

# Default version if not specified
if(NOT DEFINED NGHTTP2_ASIO_VERSION)
    set(NGHTTP2_ASIO_VERSION "v1.62.1")
endif()

message(STATUS "Fetching nghttp2-asio version: ${NGHTTP2_ASIO_VERSION}")

# Fetch nghttp2-asio from the separate repository (moved from main nghttp2 in 2022)
FetchContent_Declare(
    nghttp2_asio
    URL https://github.com/nghttp2/nghttp2-asio/archive/refs/heads/main.tar.gz
    PATCH_COMMAND ${CMAKE_CURRENT_LIST_DIR}/patch_nghttp2_asio.sh .
)

# Configure nghttp2 build options
set(ENABLE_ASIO_LIB ON CACHE BOOL "Build nghttp2 asio library" FORCE)
set(ENABLE_APP OFF CACHE BOOL "Build nghttp2 applications" FORCE)
set(ENABLE_HPACK_TOOLS OFF CACHE BOOL "Build HPACK tools" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)

FetchContent_MakeAvailable(nghttp2_asio)
