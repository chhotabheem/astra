set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# MongoDB drivers are pre-installed in Docker image
find_package(mongoc REQUIRED)
find_package(bson REQUIRED)
find_package(bsoncxx REQUIRED)
find_package(mongocxx REQUIRED)

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
    PRIVATE
        astra_sanitizers
    PUBLIC
        observability
        mongo::mongocxx_static
        mongo::bsoncxx_static
)

# Test executable
# GTest Integration
astra_add_test(
    TARGET mongo_integration_test
    SOURCES tests/mongo_integration_test.cpp
    LIBRARIES mongoclient
)

astra_add_test(
    TARGET mongo_mock_test
    SOURCES tests/mongo_mock_test.cpp
    LIBRARIES mongoclient
)

# Main application executable
add_executable(mongo_app main.cpp)
target_link_libraries(mongo_app PRIVATE mongoclient)
