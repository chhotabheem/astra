# Domain tests

add_executable(uri_shortener_domain_test
    short_code_test.cpp
    original_url_test.cpp
    expiration_policy_test.cpp
    short_link_test.cpp
    shorten_link_test.cpp
    resolve_link_test.cpp
    delete_link_test.cpp
)

target_link_libraries(uri_shortener_domain_test
    PRIVATE
        uri_shortener_domain
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(uri_shortener_domain_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/uri_shortener
        ${CMAKE_SOURCE_DIR}/libs/core/outcome/include
)

gtest_discover_tests(uri_shortener_domain_test)

# Domain benchmarks
if(ENABLE_BENCHMARK)
    add_executable(domain_benchmark domain_benchmark.cpp)
    target_link_libraries(domain_benchmark PRIVATE uri_shortener_domain benchmark::benchmark)
    target_include_directories(domain_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/libs/core/outcome/include)
    add_test(NAME domain_benchmark COMMAND domain_benchmark)
    set_tests_properties(domain_benchmark PROPERTIES LABELS bench)
endif()
