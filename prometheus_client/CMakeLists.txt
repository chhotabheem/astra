cmake_minimum_required(VERSION 3.25)
project(prometheus_client)

include(FetchContent)

FetchContent_Declare(
    prometheus-cpp
    URL https://github.com/jupp0r/prometheus-cpp/archive/refs/tags/v1.3.0.tar.gz
)

# Disable pull mode (we likely only need push or just the core)
# But for now, let's keep defaults or disable what we don't need to save build time
set(ENABLE_PULL OFF CACHE BOOL "Enable pull" FORCE)
set(ENABLE_PUSH OFF CACHE BOOL "Enable push" FORCE) # Enable if needed later
set(ENABLE_TESTING OFF CACHE BOOL "Enable testing" FORCE)

FetchContent_MakeAvailable(prometheus-cpp)

add_library(prometheus_client src/PrometheusManager.cpp)

target_include_directories(prometheus_client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

target_link_libraries(prometheus_client
    PUBLIC
        prometheus-cpp::core
)

target_compile_features(prometheus_client PUBLIC cxx_std_17)

enable_testing()
# GTest Integration
astra_add_test(
    TARGET prometheus_gtest
    SOURCES tests/prometheus_gtest.cpp
    LIBRARIES prometheus_client
)
