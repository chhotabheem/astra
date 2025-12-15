# Tests for URI Shortener

# Domain tests
add_executable(uri_shortener_domain_test
    short_code_test.cpp
    original_url_test.cpp
    expiration_policy_test.cpp
    short_link_test.cpp
    shorten_link_test.cpp
    resolve_link_test.cpp
    delete_link_test.cpp
    app_test.cpp
    observable_repository_test.cpp
    uri_shortener_handlers_test.cpp
    observable_handler_test.cpp
)

target_link_libraries(uri_shortener_domain_test
    PRIVATE
        uri_shortener_domain
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(uri_shortener_domain_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}/libs/core/outcome/include
)

gtest_discover_tests(uri_shortener_domain_test)

# Proto config tests
add_executable(proto_config_test proto_config_test.cpp)
target_link_libraries(proto_config_test PRIVATE uri_shortener_config GTest::gtest GTest::gtest_main)
gtest_discover_tests(proto_config_test)

add_executable(proto_config_extended_test proto_config_extended_test.cpp)
target_link_libraries(proto_config_extended_test PRIVATE uri_shortener_config GTest::gtest GTest::gtest_main)
gtest_discover_tests(proto_config_extended_test)

add_executable(proto_config_loader_test proto_config_loader_test.cpp)
target_link_libraries(proto_config_loader_test PRIVATE uri_shortener_config GTest::gtest GTest::gtest_main)
target_include_directories(proto_config_loader_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../config/include)
gtest_discover_tests(proto_config_loader_test)

# HTTP Data Service Adapter tests
add_executable(http_data_service_adapter_test
    ../service/tests/http_data_service_adapter_test.cpp
)
target_link_libraries(http_data_service_adapter_test
    PRIVATE
        uri_shortener_domain
        http2client
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)
target_include_directories(http_data_service_adapter_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../service/include
        ${CMAKE_SOURCE_DIR}/libs/net/http/v2/client/include
)
gtest_discover_tests(http_data_service_adapter_test)

# Data Service Handler tests
add_executable(data_service_handler_test
    ../service/tests/data_service_handler_test.cpp
)
target_link_libraries(data_service_handler_test
    PRIVATE
        uri_shortener_domain
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)
target_include_directories(data_service_handler_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../service/include
        ${CMAKE_SOURCE_DIR}/libs/core/execution/include
        ${CMAKE_SOURCE_DIR}/libs/core/observability/include
)
gtest_discover_tests(data_service_handler_test)

# Benchmarks (only when Benchmark is enabled)
if(ENABLE_BENCHMARK)
    # Service layer benchmarks
    add_executable(service_benchmark ../service/tests/service_benchmark.cpp)
    target_link_libraries(service_benchmark PRIVATE uri_shortener_domain benchmark::benchmark)
    target_include_directories(service_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/libs/core/outcome/include)
    add_test(NAME service_benchmark COMMAND service_benchmark)
    set_tests_properties(service_benchmark PROPERTIES LABELS bench)
    
    # Domain layer benchmarks
    add_executable(domain_benchmark ../domain/tests/domain_benchmark.cpp)
    target_link_libraries(domain_benchmark PRIVATE uri_shortener_domain benchmark::benchmark)
    target_include_directories(domain_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/libs/core/outcome/include)
    add_test(NAME domain_benchmark COMMAND domain_benchmark)
    set_tests_properties(domain_benchmark PROPERTIES LABELS bench)
endif()
