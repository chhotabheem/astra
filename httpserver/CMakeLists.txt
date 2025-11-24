cmake_minimum_required(VERSION 3.15)
project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

# Add nghttp2-asio via FetchContent
add_subdirectory(nghttp2)

# Find optional dependencies
find_package(simdjson CONFIG QUIET)
find_package(Boost REQUIRED COMPONENTS system thread chrono)

set(HTTPSERVER_SOURCES
    src/HttpServerFactory.cpp
    src/Validator.cpp
)

# Note: NgHttp2 implementation files (.cpp) will be added when implemented
# Currently only placeholder headers exist


# Create the library
add_library(httpserver STATIC ${HTTPSERVER_SOURCES})

# Set C++ standard
target_compile_features(httpserver PUBLIC cxx_std_17)

# Include directories
target_include_directories(httpserver
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src/nghttp2
)

# Link libraries
target_link_libraries(httpserver 
    PUBLIC
        Boost::system
        Boost::thread
        Boost::chrono
    PRIVATE
        nghttp2_asio
)

# Add compile definition for nghttp2
target_compile_definitions(httpserver PUBLIC HAS_NGHTTP2)

# Optional simdjson support
if (simdjson_FOUND)
    target_link_libraries(httpserver PRIVATE simdjson::simdjson)
    target_compile_definitions(httpserver PRIVATE HAS_SIMDJSON)
else()
    message(STATUS "simdjson not found, using fallback validation")
endif()

# Test executable for HTTP/2 server
add_executable(test_http2_server tests/test_http2_server.cpp)

target_compile_features(test_http2_server PRIVATE cxx_std_17)

target_link_libraries(test_http2_server
    PRIVATE
        nghttp2_asio
        Boost::system
        Boost::thread
        Boost::chrono
)
