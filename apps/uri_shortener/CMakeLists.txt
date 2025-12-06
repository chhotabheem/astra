# URI Shortener - Domain-Driven Design
# Pure domain code with no infrastructure dependencies

# -----------------------------------------------------------------------------
# Domain Library (core business logic)
# -----------------------------------------------------------------------------
add_library(uri_shortener_domain STATIC
    # Value Objects
    domain/value_objects/ShortCode.cpp
    domain/value_objects/OriginalUrl.cpp
    domain/value_objects/ExpirationPolicy.cpp
    # Entities
    domain/entities/ShortLink.cpp
    # Application Use Cases
    application/use_cases/ShortenLink.cpp
    application/use_cases/ResolveLink.cpp
    application/use_cases/DeleteLink.cpp
    # App
    UriShortenerApp.cpp
)

target_include_directories(uri_shortener_domain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/core/outcome/include
        ${CMAKE_SOURCE_DIR}/libs/net/http/v2/server/include
        ${CMAKE_SOURCE_DIR}/libs/net/router/include
        ${CMAKE_SOURCE_DIR}/libs/core/observability/include
)

target_link_libraries(uri_shortener_domain
    PUBLIC
        http2server
        astra_router
        observability
)

target_compile_features(uri_shortener_domain PUBLIC cxx_std_20)

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
        tests/test_short_code.cpp
        tests/test_original_url.cpp
        tests/test_expiration_policy.cpp
        tests/test_short_link.cpp
        tests/test_shorten_link.cpp
        tests/test_resolve_link.cpp
        tests/test_delete_link.cpp
        tests/test_app.cpp
        tests/test_observable_repository.cpp
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
endif()
