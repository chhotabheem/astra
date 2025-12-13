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
