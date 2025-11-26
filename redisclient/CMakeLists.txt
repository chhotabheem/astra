cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
find_package(PkgConfig REQUIRED)

# Find system-installed hiredis using pkg-config
pkg_check_modules(HIREDIS REQUIRED hiredis)

# Fetch redis-plus-plus
FetchContent_Declare(
    redis-plus-plus
    URL https://github.com/sewenew/redis-plus-plus/archive/refs/tags/1.3.12.tar.gz
)

# Configure redis-plus-plus
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "" FORCE)

# Point redis-plus-plus to system hiredis
# redis-plus-plus uses CMAKE_PREFIX_PATH or standard paths to find hiredis
# Since we found it via pkg-config, we might need to help it if it uses find_package internally
# But typically it looks in standard locations.
# We can set HIREDIS_HEADER and HIREDIS_LIB to help it.

if(HIREDIS_FOUND)
    # HIREDIS_INCLUDE_DIRS contains the path to hiredis headers
    # HIREDIS_LIBRARIES contains the library name
    # HIREDIS_LIBRARY_DIRS contains the path to libraries
    
    # redis-plus-plus CMakeLists.txt looks for HIREDIS_HEADER and HIREDIS_LIB variables
    # It expects HIREDIS_HEADER to be the directory containing "hiredis/hiredis.h"
    # But pkg-config usually gives the include dir where hiredis.h is directly present (e.g. /usr/include/hiredis)
    # or /usr/include.
    
    # Let's check what pkg-config returned
    message(STATUS "HIREDIS_INCLUDE_DIRS: ${HIREDIS_INCLUDE_DIRS}")
    
    # If HIREDIS_INCLUDE_DIRS points to /usr/include/hiredis, then hiredis.h is inside it.
    # redis-plus-plus expects <hiredis/hiredis.h>, so it needs /usr/include.
    
    # We will let redis-plus-plus try to find it first.
endif()

FetchContent_MakeAvailable(redis-plus-plus)

# Create library
add_library(redisclient STATIC
    src/RedisClient.cpp
)

target_include_directories(redisclient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(redisclient
    PUBLIC
        redis++::redis++
)

# Enable testing
enable_testing()

# Test executable
add_executable(test_redisclient
    tests/test_redis_unit.cpp
)

target_link_libraries(test_redisclient
    PRIVATE
        redisclient
)

# Add tests
add_test(NAME RedisClientBasicTest COMMAND test_redisclient)
