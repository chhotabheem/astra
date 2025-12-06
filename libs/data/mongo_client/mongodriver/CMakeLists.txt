cmake_minimum_required(VERSION 3.25)

add_definitions(-D_ISOC11_SOURCE)

# Default version if not specified
if(NOT DEFINED MONGO_CXX_DRIVER_VERSION)
    set(MONGO_CXX_DRIVER_VERSION "r4.1.4")
endif()

message(STATUS "Fetching MongoDB C++ Driver version: ${MONGO_CXX_DRIVER_VERSION}")

# MongoDB C driver is pre-installed in Docker image
find_package(mongoc REQUIRED)
find_package(bson REQUIRED)

message(STATUS "Using system-installed MongoDB C Driver version ${mongoc_VERSION}")

# Fetch only the C++ driver (uses system C driver)
include(FetchContent)
FetchContent_Declare(
  mongo_cxx_driver
  URL https://github.com/mongodb/mongo-cxx-driver/releases/download/${MONGO_CXX_DRIVER_VERSION}/mongo-cxx-driver-${MONGO_CXX_DRIVER_VERSION}.tar.gz
)

# Disable warnings as errors to prevent build failures with Sanitizers
set(ENABLE_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ENABLE_MAINTAINER_FLAGS OFF CACHE BOOL "" FORCE)

# Build static libraries (industry standard for containerized deployments)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(mongo_cxx_driver)
