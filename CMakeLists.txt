cmake_minimum_required(VERSION 3.25)

project(astra
    VERSION 1.0.0
    DESCRIPTION "High-performance C++ microservice platform"
    LANGUAGES C CXX
)

# Global Compiler Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output Directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Custom Modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Valgrind must be included BEFORE CTest to set MEMORYCHECK variables
include(Valgrind)
# Testing
enable_testing()

# Testing (CTest generates DartConfiguration.tcl needed for memcheck)
include(CTest)
include(GoogleTest)
find_package(GTest REQUIRED)  # Pre-installed in Docker image
include(AddAstraTest)
include(Sanitizers)
include(Coverage)

# FuzzTest Support (downloads via FetchContent when ENABLE_FUZZTEST=ON)
include(FuzzTest)

# Benchmark Support (uses pre-installed Google Benchmark when ENABLE_BENCHMARK=ON)
include(Benchmark)

# Subdirectories
add_subdirectory(libs)
add_subdirectory(apps)

# Container build targets (image-release, push-release, etc.)
include(ContainerTargets)
