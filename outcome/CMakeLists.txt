cmake_minimum_required(VERSION 3.25)

# Outcome library (header-only)
add_library(outcome INTERFACE)
target_include_directories(outcome INTERFACE include/)
target_compile_features(outcome INTERFACE cxx_std_17)

# Tests
enable_testing()

astra_add_test(
    TARGET outcome_test
    SOURCES tests/result_test.cpp
    INCLUDE_DIRS include
    LIBRARIES outcome
)
