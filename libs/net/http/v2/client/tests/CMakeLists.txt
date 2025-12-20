include(GoogleTest)

astra_add_test(
    TARGET test_http2_client
    SOURCES 
        http2_client_test.cpp
    LIBRARIES http2client observability
)

# Fuzz tests (only when FuzzTest is enabled)
if(ENABLE_FUZZTEST)
    add_executable(http2_client_fuzz_test http2_client_fuzz_test.cpp)
    target_link_libraries(http2_client_fuzz_test PRIVATE http2client)
    link_fuzztest(http2_client_fuzz_test)
    gtest_discover_tests(http2_client_fuzz_test PROPERTIES LABELS fuzz)
endif()

# Benchmarks (only when Benchmark is enabled)
if(ENABLE_BENCHMARK)
    add_executable(http2_client_benchmark http2_client_benchmark.cpp)
    target_link_libraries(http2_client_benchmark PRIVATE http2client benchmark::benchmark)
    add_test(NAME http2_client_benchmark COMMAND http2_client_benchmark)
    set_tests_properties(http2_client_benchmark PROPERTIES LABELS bench)
endif()
