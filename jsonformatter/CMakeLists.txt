cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch RapidJSON
include(FetchContent)

# Disable RapidJSON tests, examples, and docs
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    rapidjson
    URL https://github.com/Tencent/rapidjson/archive/refs/tags/v1.1.0.tar.gz
)
FetchContent_MakeAvailable(rapidjson)

# Create library
add_library(jsonformatter STATIC
    src/JsonFormatter.cpp
)

target_include_directories(jsonformatter
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${rapidjson_SOURCE_DIR}/include
)

# Enable testing
enable_testing()

# Test executable
add_executable(test_jsonformatter
    tests/test_jsonformatter.cpp
)

target_link_libraries(test_jsonformatter
    PRIVATE
        jsonformatter
)

# Add tests
add_test(NAME jsonformatter_basic_test COMMAND test_jsonformatter)
