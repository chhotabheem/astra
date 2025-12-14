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
