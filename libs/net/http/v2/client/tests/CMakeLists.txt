include(GoogleTest)

# Http2ClientPool tests
add_executable(test_http2_client_pool
    http2_client_pool_test.cpp
)

target_link_libraries(test_http2_client_pool
    PRIVATE
        http2client
        GTest::gtest
        GTest::gtest_main
        astra_sanitizers
)

gtest_discover_tests(test_http2_client_pool)

# Benchmarks (only when Benchmark is enabled)
if(ENABLE_BENCHMARK)
    add_executable(http2_client_benchmark http2_client_benchmark.cpp)
    target_link_libraries(http2_client_benchmark PRIVATE http2client benchmark::benchmark)
    add_test(NAME http2_client_benchmark COMMAND http2_client_benchmark)
    set_tests_properties(http2_client_benchmark PROPERTIES LABELS bench)
endif()
