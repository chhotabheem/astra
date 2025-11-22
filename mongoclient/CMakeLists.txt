cmake_minimum_required(VERSION 3.15)
project(MongoClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_library(mongoclient IMongoClient.h MongoClient.cpp)

# Add MongoDB Driver
add_subdirectory(mongodriver)

# find_package(mongocxx REQUIRED) # Not needed as we build it from source
# find_package(bsoncxx REQUIRED)

# Link libraries
target_link_libraries(mongoclient 
    PUBLIC 
        logger
        mongocxx_shared 
        bsoncxx_shared
)

# Include current directory for headers
target_include_directories(mongoclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Test executable
add_executable(test_mongoclient tests/test_mongoclient.cpp)
target_link_libraries(test_mongoclient PRIVATE mongoclient)

add_test(NAME MongoClientTest COMMAND test_mongoclient)

# Main application executable
add_executable(mongo_app main.cpp)
target_link_libraries(mongo_app PRIVATE mongoclient)