cmake_minimum_required(VERSION 3.25)
project(zookeeperclient)

# Try to find the Zookeeper C library
find_path(ZOOKEEPER_INCLUDE_DIR zookeeper/zookeeper.h)
find_library(ZOOKEEPER_LIBRARY zookeeper_mt)

if (ZOOKEEPER_INCLUDE_DIR AND ZOOKEEPER_LIBRARY)
    message(STATUS "Found Zookeeper: ${ZOOKEEPER_LIBRARY}")
else()
    message(WARNING "Zookeeper library not found. Building with mock implementation for compilation.")
    set(ZOOKEEPER_MOCK TRUE)
endif()

add_library(zookeeperclient src/ZookeeperClient.cpp)

target_include_directories(zookeeperclient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

if (NOT ZOOKEEPER_MOCK)
    target_include_directories(zookeeperclient PRIVATE ${ZOOKEEPER_INCLUDE_DIR})
    target_link_libraries(zookeeperclient PRIVATE ${ZOOKEEPER_LIBRARY})
    target_compile_definitions(zookeeperclient PRIVATE HAS_ZOOKEEPER)
else()
    target_compile_definitions(zookeeperclient PRIVATE USE_ZOOKEEPER_MOCK)
endif()

target_compile_features(zookeeperclient PUBLIC cxx_std_17)

enable_testing()
# GTest Integration
astra_add_test(
    TARGET zookeeper_gtest
    SOURCES tests/zookeeper_gtest.cpp
    LIBRARIES zookeeperclient
)
