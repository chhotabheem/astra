cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost
find_package(Boost REQUIRED)

# Create library
add_library(json STATIC
    src/JsonWriter.cpp
    src/JsonDocument.cpp
)

target_include_directories(json
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(json
    PRIVATE
        Boost::headers
)

# Enable testing
enable_testing()

# Test executable: Writer
add_executable(test_json_writer
    tests/json_writer_test.cpp
)

target_link_libraries(test_json_writer
    PRIVATE
        json
)

# Test executable: Document (Reader)
add_executable(test_json_document
    tests/json_document_test.cpp
)

target_link_libraries(test_json_document
    PRIVATE
        json
)

# Add tests
add_test(NAME json_writer_test COMMAND test_json_writer)
add_test(NAME json_document_test COMMAND test_json_document)
