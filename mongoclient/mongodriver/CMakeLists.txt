cmake_minimum_required(VERSION 3.25)

add_definitions(-D_ISOC11_SOURCE)

# Default version if not specified
if(NOT DEFINED MONGO_CXX_DRIVER_VERSION)
    set(MONGO_CXX_DRIVER_VERSION "r4.1.4")
endif()

message(STATUS "Fetching MongoDB C++ Driver version: ${MONGO_CXX_DRIVER_VERSION}")

# Try to find system-installed MongoDB C driver 2.x first
find_package(mongoc-2.0 QUIET)
find_package(bson-2.0 QUIET)

if(mongoc-2.0_FOUND AND bson-2.0_FOUND)
    message(STATUS "Using system-installed MongoDB C Driver version ${mongoc-2.0_VERSION}")
    message(STATUS "  libmongoc found via CMake config")
    message(STATUS "  libbson found via CMake config")
    
    # Now fetch the C++ driver (which will use the system C driver)
    include(FetchContent)
    FetchContent_Declare(
      mongo_cxx_driver
      URL https://github.com/mongodb/mongo-cxx-driver/releases/download/${MONGO_CXX_DRIVER_VERSION}/mongo-cxx-driver-${MONGO_CXX_DRIVER_VERSION}.tar.gz
    )
    
    # Disable warnings as errors to prevent build failures with Sanitizers
    set(ENABLE_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    set(ENABLE_MAINTAINER_FLAGS OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(mongo_cxx_driver)
else()
    message(STATUS "System MongoDB C Driver not found, downloading from source...")
    message(STATUS "  (This will take ~20 minutes on first build)")
    
    # Fallback: Download both C and C++ drivers
    include(FetchContent)
    FetchContent_Declare(
      mongo_cxx_driver
      URL https://github.com/mongodb/mongo-cxx-driver/releases/download/${MONGO_CXX_DRIVER_VERSION}/mongo-cxx-driver-${MONGO_CXX_DRIVER_VERSION}.tar.gz
    )
    
    set(ENABLE_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    set(ENABLE_MAINTAINER_FLAGS OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(mongo_cxx_driver)
endif()
