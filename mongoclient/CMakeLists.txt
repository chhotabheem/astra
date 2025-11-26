cmake_minimum_required(VERSION 3.15)
project(MongoClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# Add MongoDB Driver
add_subdirectory(mongodriver)

# Create library
add_library(mongoclient STATIC
    src/MongoClient.cpp
)

target_include_directories(mongoclient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(mongoclient
    PUBLIC
        logger
        mongocxx_static
        bsoncxx_static
)

# Test executable
add_executable(test_mongoclient tests/test_mongoclient.cpp)
target_link_libraries(test_mongoclient PRIVATE mongoclient)

add_test(NAME MongoClientTest COMMAND test_mongoclient)

# Main application executable
add_executable(mongo_app main.cpp)
target_link_libraries(mongo_app PRIVATE mongoclient)
