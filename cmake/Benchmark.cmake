# Benchmark.cmake - Google Benchmark integration
# Only active when ENABLE_BENCHMARK=ON (set by clang-bench preset)

option(ENABLE_BENCHMARK "Enable Google Benchmark" OFF)

if(ENABLE_BENCHMARK)
    find_package(benchmark REQUIRED)
    message(STATUS "Benchmark: Enabled")
else()
    message(STATUS "Benchmark: Disabled")
endif()
