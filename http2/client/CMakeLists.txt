cmake_minimum_required(VERSION 3.25)

# Sources
set(HTTP2CLIENT_SOURCES
    src/Http2Client.cpp
)

find_package(Boost REQUIRED COMPONENTS system thread chrono)

# Create the library
add_library(http2client STATIC ${HTTP2CLIENT_SOURCES})

# Set C++ standard
target_compile_features(http2client PUBLIC cxx_std_17)

# Include directories
target_include_directories(http2client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link libraries
# Note: nghttp2_asio is expected to be available from the parent project or http2server
target_link_libraries(http2client 
    PUBLIC
        logger
        Boost::system
        Boost::thread
        Boost::chrono
    PRIVATE
        nghttp2_asio
)

# Example executable
add_executable(simple_client examples/simple_client.cpp)
target_include_directories(simple_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(simple_client PRIVATE http2client)
