cmake_minimum_required(VERSION 3.15)

include(FetchContent)

add_definitions(-D_ISOC11_SOURCE)
# Default version if not specified
if(NOT DEFINED MONGO_CXX_DRIVER_VERSION)
    set(MONGO_CXX_DRIVER_VERSION "r4.1.4")
endif()

message(STATUS "Fetching MongoDB C++ Driver version: ${MONGO_CXX_DRIVER_VERSION}")

FetchContent_Declare(
  mongo_cxx_driver
  URL https://github.com/mongodb/mongo-cxx-driver/releases/download/${MONGO_CXX_DRIVER_VERSION}/mongo-cxx-driver-${MONGO_CXX_DRIVER_VERSION}.tar.gz

)

# We need to ensure the C driver is also available or fetched by the C++ driver.
# The C++ driver's CMake build system usually looks for libmongoc using find_package.
# If it's not found, we might need to fetch it too, but let's see if the driver handles it.
# For now, we'll assume the system libraries or the driver's internal logic handles it.

FetchContent_MakeAvailable(mongo_cxx_driver)
