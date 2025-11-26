cmake_minimum_required(VERSION 3.15)

# nghttp2-asio is built by parent http2/CMakeLists.txt

find_package(Boost REQUIRED COMPONENTS system thread chrono)

# Sources
set(HTTP2SERVER_SOURCES
    src/Http2Server.cpp
    src/Http2Request.cpp
    src/Http2Response.cpp
    src/backend/nghttp2/NgHttp2Server.cpp
)

# Create the library
add_library(http2server STATIC ${HTTP2SERVER_SOURCES})

# Set C++ standard
target_compile_features(http2server PUBLIC cxx_std_17)

# Include directories
target_include_directories(http2server
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link libraries
target_link_libraries(http2server 
    PUBLIC
        http_abstractions
    PRIVATE 
        Boost::system 
        Boost::thread 
        Boost::chrono
        nghttp2_asio
        logger
)

# Add compile definition for nghttp2
target_compile_definitions(http2server PRIVATE HAS_NGHTTP2)

# Example executable
add_executable(simple_server examples/simple_server.cpp)
target_link_libraries(simple_server PRIVATE http2server router)

# Enable testing
enable_testing()

# Test executable
add_executable(test_http2server_unit tests/test_http2server_unit.cpp)
target_link_libraries(test_http2server_unit PRIVATE http2server)

# Register test with CTest
add_test(NAME Http2ServerUnitTests COMMAND test_http2server_unit)
