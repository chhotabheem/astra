cmake_minimum_required(VERSION 3.15)

include(FetchContent)

# Default version if not specified
if(NOT DEFINED NGHTTP2_ASIO_VERSION)
    set(NGHTTP2_ASIO_VERSION "v1.62.1")
endif()

message(STATUS "Fetching nghttp2-asio version: ${NGHTTP2_ASIO_VERSION}")

# Fetch nghttp2-asio from GitHub
FetchContent_Declare(
    nghttp2_asio
    GIT_REPOSITORY https://github.com/nghttp2/nghttp2.git
    GIT_TAG        ${NGHTTP2_ASIO_VERSION}
    SOURCE_SUBDIR  src  # nghttp2-asio is in the src directory
)

# Configure nghttp2 build options
set(ENABLE_ASIO_LIB ON CACHE BOOL "Build nghttp2 asio library" FORCE)
set(ENABLE_APP OFF CACHE BOOL "Build nghttp2 applications" FORCE)
set(ENABLE_HPACK_TOOLS OFF CACHE BOOL "Build HPACK tools" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)

FetchContent_MakeAvailable(nghttp2_asio)
