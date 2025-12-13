# redis-plus-plus is pre-installed in Docker image
find_package(redis++ REQUIRED)

# Create library
add_library(redisclient STATIC
    src/RedisClient.cpp
)

target_include_directories(redisclient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(redisclient
    PRIVATE
        astra_sanitizers
    PUBLIC
        redis++::redis++
)

# Enable testing
enable_testing()

# Test executable
astra_add_test(
    TARGET redis_integration_test
    SOURCES tests/redis_integration_test.cpp
    LIBRARIES redisclient
)

astra_add_test(
    TARGET redis_mock_test
    SOURCES tests/redis_mock_test.cpp
    LIBRARIES redisclient
)
