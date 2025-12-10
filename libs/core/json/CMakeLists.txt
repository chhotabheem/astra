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
        astra_sanitizers
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
        GTest::gtest_main
)

# Test executable: Document (Reader)
add_executable(test_json_document
    tests/json_document_test.cpp
)

target_link_libraries(test_json_document
    PRIVATE
        json
        GTest::gtest_main
)

# Add tests
include(GoogleTest)
gtest_discover_tests(test_json_writer)
gtest_discover_tests(test_json_document)
