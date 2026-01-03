astra_add_test(
    TARGET router_test
    SOURCES router_test.cpp
    LIBRARIES astra_router
)

# Fuzz tests (only when FuzzTest is enabled)
if(ENABLE_FUZZTEST)
    add_executable(router_fuzz_test router_fuzz_test.cpp)
    target_link_libraries(router_fuzz_test PRIVATE astra_router)
    link_fuzztest(router_fuzz_test)
    gtest_discover_tests(router_fuzz_test PROPERTIES LABELS fuzz)
endif()

# Benchmarks (only when Benchmark is enabled)
if(ENABLE_BENCHMARK)
    add_executable(router_benchmark router_benchmark.cpp)
    target_link_libraries(router_benchmark PRIVATE astra_router benchmark::benchmark)
    add_test(NAME router_benchmark COMMAND router_benchmark)
    set_tests_properties(router_benchmark PROPERTIES LABELS bench)
endif()
