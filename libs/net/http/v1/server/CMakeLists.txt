find_package(Boost REQUIRED COMPONENTS system thread)

set(HTTP1_1SERVER_SOURCES
    src/Http1Server.cpp
    src/Http1Request.cpp
    src/Http1Response.cpp
)

add_library(http1.1server ${HTTP1_1SERVER_SOURCES})

target_include_directories(http1.1server
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

target_link_libraries(http1.1server
    PRIVATE
        astra_sanitizers
        Boost::system
        Boost::thread
    PUBLIC
        astra_router
)

target_compile_features(http1.1server PUBLIC cxx_std_17)

enable_testing()

astra_add_test(
    TARGET http1_server_integration_test
    SOURCES tests/http1_server_integration_test.cpp
    LIBRARIES http1.1server
)
