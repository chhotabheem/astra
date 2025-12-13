# URI Shortener - Domain-Driven Design
# Pure domain code with no infrastructure dependencies

# -----------------------------------------------------------------------------
# Protobuf Config Generation
# -----------------------------------------------------------------------------
find_package(Protobuf REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/config/uri_shortener.proto)

# Create a library for generated protobuf code
add_library(uri_shortener_config STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(uri_shortener_config PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(uri_shortener_config PUBLIC protobuf::libprotobuf)
target_compile_features(uri_shortener_config PUBLIC cxx_std_17)

# -----------------------------------------------------------------------------
# Domain Library (core business logic)
# -----------------------------------------------------------------------------
add_library(uri_shortener_domain STATIC
    # Domain - Value Objects and Entities
    domain/src/ShortCode.cpp
    domain/src/OriginalUrl.cpp
    domain/src/ExpirationPolicy.cpp
    domain/src/ShortLink.cpp
    # Service - Use Cases
    service/src/ShortenLink.cpp
    service/src/ResolveLink.cpp
    service/src/DeleteLink.cpp
    # Service - Message Handlers
    service/src/UriShortenerRequestHandler.cpp
    service/src/UriShortenerMessageHandler.cpp
    service/src/ObservableMessageHandler.cpp
    service/src/ObservableRequestHandler.cpp
    # Service - App
    service/src/UriShortenerApp.cpp
)

target_include_directories(uri_shortener_domain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/domain/include
        ${CMAKE_CURRENT_SOURCE_DIR}/service/include
        ${CMAKE_CURRENT_SOURCE_DIR}/config/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/core/outcome/include
        ${CMAKE_SOURCE_DIR}/libs/net/http/v2/server/include
        ${CMAKE_SOURCE_DIR}/libs/net/http/v2/server/src
        ${CMAKE_SOURCE_DIR}/libs/net/router/include
        ${CMAKE_SOURCE_DIR}/libs/core/observability/include
        ${CMAKE_SOURCE_DIR}/libs/core/execution/include
        ${CMAKE_SOURCE_DIR}/libs/core/resilience/include
)

target_link_libraries(uri_shortener_domain
    PRIVATE
        astra_sanitizers
    PUBLIC
        uri_shortener_config
        http2server
        astra_router
        observability
        astra_execution
        resilience
)

target_compile_features(uri_shortener_domain PUBLIC cxx_std_17)

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------
add_executable(uri_shortener main.cpp)

target_link_libraries(uri_shortener
    PRIVATE
        uri_shortener_domain
)

target_include_directories(uri_shortener
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs/core/outcome/include
)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
if(BUILD_TESTING)
    add_executable(uri_shortener_domain_test
        tests/short_code_test.cpp
        tests/original_url_test.cpp
        tests/expiration_policy_test.cpp
        tests/short_link_test.cpp
        tests/shorten_link_test.cpp
        tests/resolve_link_test.cpp
        tests/delete_link_test.cpp
        tests/app_test.cpp
        tests/observable_repository_test.cpp
        tests/uri_shortener_handlers_test.cpp
        tests/observable_handler_test.cpp
    )

    target_link_libraries(uri_shortener_domain_test
        PRIVATE
            uri_shortener_domain
            GTest::gtest
            GTest::gtest_main
    )

    target_include_directories(uri_shortener_domain_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/libs/core/outcome/include
    )

    gtest_discover_tests(uri_shortener_domain_test)

    # Proto config tests (80+ unique test cases)
    add_executable(proto_config_test
        tests/proto_config_test.cpp
    )

    target_link_libraries(proto_config_test
        PRIVATE
            uri_shortener_config
            GTest::gtest
            GTest::gtest_main
    )

    gtest_discover_tests(proto_config_test)

    # Proto config extended tests (125+ additional unique test cases)
    add_executable(proto_config_extended_test
        tests/proto_config_extended_test.cpp
    )

    target_link_libraries(proto_config_extended_test
        PRIVATE
            uri_shortener_config
            GTest::gtest
            GTest::gtest_main
    )

    gtest_discover_tests(proto_config_extended_test)

    # Proto config loader tests
    add_executable(proto_config_loader_test
        tests/proto_config_loader_test.cpp
    )

    target_link_libraries(proto_config_loader_test
        PRIVATE
            uri_shortener_config
            GTest::gtest
            GTest::gtest_main
    )

    target_include_directories(proto_config_loader_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/config/include
    )

    gtest_discover_tests(proto_config_loader_test)

    # Config migrator tests
    add_executable(config_migrator_test
        tests/config_migrator_test.cpp
    )

    target_link_libraries(config_migrator_test
        PRIVATE
            uri_shortener_config
            GTest::gtest
            GTest::gtest_main
    )

    target_include_directories(config_migrator_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/config/include
    )

    gtest_discover_tests(config_migrator_test)
endif()

