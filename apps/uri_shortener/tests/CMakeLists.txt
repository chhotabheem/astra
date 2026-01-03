# Integration tests for URI Shortener

add_executable(uri_shortener_integration_test
    app_test.cpp
    app_builder_test.cpp
)

target_link_libraries(uri_shortener_integration_test
    PRIVATE
        uri_shortener_domain
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(uri_shortener_integration_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}/libs/core/outcome/include
)

gtest_discover_tests(uri_shortener_integration_test)
