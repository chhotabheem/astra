# Protobuf generation
find_package(Protobuf REQUIRED)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS config/http2client.proto)

# Sources
set(HTTP2CLIENT_SOURCES
    src/Http2Client.cpp
    src/Http2ClientResponse.cpp
    src/NgHttp2Client.cpp
    src/ClientRegistry.cpp
    ${PROTO_SRCS}
)

find_package(Boost REQUIRED COMPONENTS system thread chrono)

# Create the library
add_library(http2client STATIC ${HTTP2CLIENT_SOURCES})

# Set C++ standard
target_compile_features(http2client PUBLIC cxx_std_17)

# Include directories
target_include_directories(http2client
    PUBLIC
        include
        ${CMAKE_CURRENT_BINARY_DIR}  # For generated proto headers
)

# Link libraries
find_package(OpenSSL REQUIRED)
target_link_libraries(http2client 
    PUBLIC
        protobuf::libprotobuf
        outcome
    PRIVATE
        astra_sanitizers
        nghttp2_asio
        OpenSSL::SSL
        OpenSSL::Crypto
        observability
        Boost::system
        Boost::thread
        Boost::chrono
)

# Add tests if testing is enabled
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

