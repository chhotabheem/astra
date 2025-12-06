cmake_minimum_required(VERSION 3.25)
project(uri_shortner)

find_package(Boost REQUIRED COMPONENTS system thread)

# Sources
set(URI_SHORTENER_SOURCES
    src/UriService.cpp
    src/RedisUriRepository.cpp
    src/MongoUriRepository.cpp
    src/UriController.cpp
)

# Application Executable
add_executable(uri_shortner_app main.cpp ${URI_SHORTENER_SOURCES})

# Include Directories
target_include_directories(uri_shortner_app
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link Libraries
target_link_libraries(uri_shortner_app
    PRIVATE
        astra_router
        redisclient
        mongoclient
        http1.1server
        http2server
        Boost::system
        Boost::thread
)

# Compiler Features
target_compile_features(uri_shortner_app PUBLIC cxx_std_17)

# Testing
enable_testing()
astra_add_test(
    TARGET uri_shortener_gtest
    SOURCES tests/uri_shortener_gtest.cpp ${URI_SHORTENER_SOURCES}
    INCLUDE_DIRS include src
    LIBRARIES 
        astra_router
        redisclient
        mongoclient
        http1.1server
        http2server
        Boost::system
        Boost::thread
)

# Load Generator
add_executable(load_test load_test.cpp)
target_link_libraries(load_test
    PRIVATE
        http2client
        Boost::system
        Boost::thread
)

# Mock Server for Load Testing
add_executable(mock_server mock_server.cpp ${URI_SHORTENER_SOURCES})
target_link_libraries(mock_server
    PRIVATE
        astra_router
        http1.1server
        http2server
        redisclient
        mongoclient
        Boost::system
        Boost::thread
)
target_include_directories(mock_server
    PRIVATE
        include
        src
)
