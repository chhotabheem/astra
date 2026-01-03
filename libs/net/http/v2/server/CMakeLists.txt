# nghttp2-asio is pre-installed in Docker image

# Protobuf generation
find_package(Protobuf REQUIRED)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS config/http2server.proto)

find_package(Boost REQUIRED COMPONENTS system thread chrono)

# Sources
set(HTTP2SERVER_SOURCES
    src/Http2Server.cpp
    src/Http2Request.cpp
    src/Http2Response.cpp
    src/Http2ResponseWriter.cpp
    src/NgHttp2Server.cpp
    ${PROTO_SRCS}
)

# Create the library
add_library(http2server STATIC ${HTTP2SERVER_SOURCES})

# Set C++ standard
target_compile_features(http2server PUBLIC cxx_std_17)

# Include directories
target_include_directories(http2server
    PUBLIC
        include
        ${CMAKE_CURRENT_BINARY_DIR}  # For generated proto headers
)

# Link libraries
find_package(OpenSSL REQUIRED)
target_link_libraries(http2server 
    PUBLIC
        protobuf::libprotobuf
        astra_router
        astra_execution
        outcome
    PRIVATE
        astra_utils
        astra_sanitizers
        Boost::system 
        Boost::thread 
        Boost::chrono
        nghttp2_asio
        OpenSSL::SSL
        OpenSSL::Crypto
        observability
)

# Add compile definition for nghttp2
target_compile_definitions(http2server PRIVATE HAS_NGHTTP2)

# Enable testing
enable_testing()

# Test executable
astra_add_test(
    TARGET http2_server_test
    SOURCES 
        tests/http2_server_test.cpp
        tests/http2_response_writer_test.cpp
        tests/request_handle_test.cpp
        tests/response_integration_test.cpp
        tests/handler_signature_test.cpp
    LIBRARIES http2server
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Benchmarks (only when Benchmark is enabled)
if(ENABLE_BENCHMARK)
    add_executable(http2_server_benchmark tests/http2_server_benchmark.cpp)
    target_link_libraries(http2_server_benchmark PRIVATE http2server benchmark::benchmark)
    add_test(NAME http2_server_benchmark COMMAND http2_server_benchmark)
    set_tests_properties(http2_server_benchmark PROPERTIES LABELS bench)
endif()
