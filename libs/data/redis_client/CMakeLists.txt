find_package(redis++ REQUIRED)

add_library(redisclient STATIC
    src/RedisClient.cpp
    src/SwRedisClient.cpp
)

target_include_directories(redisclient
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(redisclient
    PRIVATE
        astra_sanitizers
        observability
    PUBLIC
        redis++::redis++
)

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
