find_package(Boost REQUIRED)

add_library(json STATIC
    src/BoostJsonImpl.cpp
    src/JsonWriter.cpp
    src/JsonDocument.cpp
)

target_include_directories(json
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(json
    PRIVATE
        astra_sanitizers
        Boost::headers
)

add_executable(json_writer_test
    tests/json_writer_test.cpp
)

target_link_libraries(json_writer_test
    PRIVATE
        json
        GTest::gtest_main
)

add_executable(json_document_test
    tests/json_document_test.cpp
)

target_link_libraries(json_document_test
    PRIVATE
        json
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(json_writer_test)
gtest_discover_tests(json_document_test)
if(ENABLE_FUZZTEST)
    add_executable(json_fuzz_test tests/json_fuzz_test.cpp)
    target_link_libraries(json_fuzz_test PRIVATE json)
    link_fuzztest(json_fuzz_test)
    gtest_discover_tests(json_fuzz_test PROPERTIES LABELS fuzz)
endif()

if(ENABLE_BENCHMARK)
    add_executable(json_benchmark tests/json_benchmark.cpp)
    target_link_libraries(json_benchmark PRIVATE json benchmark::benchmark)
    add_test(NAME json_benchmark COMMAND json_benchmark)
    set_tests_properties(json_benchmark PROPERTIES LABELS bench)
endif()
