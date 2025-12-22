# Service tests

add_executable(uri_shortener_service_test
    observable_repository_test.cpp
    observable_handler_test.cpp
    uri_shortener_handlers_test.cpp
)

target_link_libraries(uri_shortener_service_test
    PRIVATE
        uri_shortener_domain
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(uri_shortener_service_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/uri_shortener
        ${CMAKE_SOURCE_DIR}/libs/core/outcome/include
)

gtest_discover_tests(uri_shortener_service_test)

# HTTP Data Service Adapter tests
add_executable(http_data_service_adapter_test http_data_service_adapter_test.cpp)
target_link_libraries(http_data_service_adapter_test
    PRIVATE
        uri_shortener_domain
        http2client
        service_discovery
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)
target_include_directories(http_data_service_adapter_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_SOURCE_DIR}/libs/net/http/v2/client/include
        ${CMAKE_SOURCE_DIR}/libs/core/service_discovery/include
)
gtest_discover_tests(http_data_service_adapter_test)

# Data Service Handler tests
add_executable(data_service_handler_test data_service_handler_test.cpp)
target_link_libraries(data_service_handler_test
    PRIVATE
        uri_shortener_domain
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)
target_include_directories(data_service_handler_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_SOURCE_DIR}/libs/core/execution/include
        ${CMAKE_SOURCE_DIR}/libs/core/observability/include
)
gtest_discover_tests(data_service_handler_test)

# Service benchmarks
if(ENABLE_BENCHMARK)
    add_executable(service_benchmark service_benchmark.cpp)
    target_link_libraries(service_benchmark PRIVATE uri_shortener_domain benchmark::benchmark)
    target_include_directories(service_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/libs/core/outcome/include)
    add_test(NAME service_benchmark COMMAND service_benchmark)
    set_tests_properties(service_benchmark PROPERTIES LABELS bench)
endif()
