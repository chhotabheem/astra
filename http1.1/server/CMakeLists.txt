cmake_minimum_required(VERSION 3.15)
project(http1.1server)

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
    PUBLIC
        Boost::system
        Boost::thread
        http_abstractions
)

target_compile_features(http1.1server PUBLIC cxx_std_17)

enable_testing()
add_executable(test_http1.1server_unit tests/test_http1.1server_unit.cpp)
target_link_libraries(test_http1.1server_unit PRIVATE http1.1server)
add_test(NAME Http1.1ServerUnitTests COMMAND test_http1.1server_unit)
