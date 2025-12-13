# URI Shortener - Domain-Driven Design

# Protobuf Config
add_subdirectory(config)

# Domain Library
add_library(uri_shortener_domain STATIC
    domain/src/ShortCode.cpp
    domain/src/OriginalUrl.cpp
    domain/src/ExpirationPolicy.cpp
    domain/src/ShortLink.cpp
    service/src/ShortenLink.cpp
    service/src/ResolveLink.cpp
    service/src/DeleteLink.cpp
    service/src/UriShortenerRequestHandler.cpp
    service/src/UriShortenerMessageHandler.cpp
    service/src/ObservableMessageHandler.cpp
    service/src/ObservableRequestHandler.cpp
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
    PRIVATE astra_sanitizers
    PUBLIC uri_shortener_config http2server astra_router observability astra_execution resilience
)

target_compile_features(uri_shortener_domain PUBLIC cxx_std_17)

# Executable
add_executable(uri_shortener main.cpp)
target_link_libraries(uri_shortener PRIVATE uri_shortener_domain)
target_include_directories(uri_shortener PRIVATE ${CMAKE_SOURCE_DIR}/libs/core/outcome/include)

# Tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
