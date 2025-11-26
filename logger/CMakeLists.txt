cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch spdlog
include(FetchContent)
FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz
)
FetchContent_MakeAvailable(spdlog)

# Create logger library
add_library(logger STATIC
    src/LoggerImpl.cpp
)

target_include_directories(logger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(logger
    PUBLIC
        spdlog::spdlog
        jsonformatter
)

# Enable testing
enable_testing()

# Test executable
add_executable(test_logger
    tests/test_logger.cpp
)

target_link_libraries(test_logger
    PRIVATE
        logger
)

# Add tests
add_test(NAME LoggerBasicTest COMMAND test_logger)
