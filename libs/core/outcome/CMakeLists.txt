# Header-only outcome library wrapper-only
add_library(outcome INTERFACE)
target_include_directories(outcome INTERFACE include/)
target_compile_features(outcome INTERFACE cxx_std_17)

# Tests
if(BUILD_TESTING)
    astra_add_test(
        TARGET outcome_test
        SOURCES tests/result_test.cpp
        INCLUDE_DIRS include
        LIBRARIES outcome
    )
endif()
