cmake_minimum_required(VERSION 3.25)
project(Astra VERSION 1.0.0 LANGUAGES C CXX)

# -----------------------------------------------------------------------------
# 1. Configuration
# -----------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug/Release/RelWithDebInfo/MinSizeRel)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Global Compiler Warnings (Production Level)
add_compile_options(-Wall -Wextra -Wpedantic)

# -----------------------------------------------------------------------------
# 2. Modules & Tools
# -----------------------------------------------------------------------------
include(CTest)          # Standard CTest
include(Sanitizers)     # Address, Thread, Memory, Undefined
include(Valgrind)       # Custom Valgrind targets

# -----------------------------------------------------------------------------
# 3. External Dependencies
# -----------------------------------------------------------------------------
include(AddGoogleTest)
include(AddAstraTest)

# -----------------------------------------------------------------------------
# 3. Subdirectories
# -----------------------------------------------------------------------------
# Utilities
add_subdirectory(jsonformatter)
add_subdirectory(logger)
add_subdirectory(exception)
add_subdirectory(concurrency)
add_subdirectory(hfsm)
add_subdirectory(boost_msm)

# Clients
add_subdirectory(redisclient)
add_subdirectory(prometheus_client)
add_subdirectory(zookeeperclient)
add_subdirectory(mongoclient)
add_subdirectory(http1.1)
add_subdirectory(uri_shortner)

# Core Server Components
add_subdirectory(router)
add_subdirectory(http2)

# Main Server (Uncomment when ready)
# add_subdirectory(server)
