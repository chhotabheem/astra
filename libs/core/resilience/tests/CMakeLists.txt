include(FetchContent)

# Use shared GoogleTest if available, otherwise fetch
if(NOT TARGET GTest::gtest)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Test executables
add_executable(load_shedder_guard_test load_shedder_guard_test.cpp)
target_link_libraries(load_shedder_guard_test PRIVATE resilience GTest::gtest_main)
add_test(NAME LoadShedderGuardTest COMMAND load_shedder_guard_test)

add_executable(atomic_load_shedder_test atomic_load_shedder_test.cpp)
target_link_libraries(atomic_load_shedder_test PRIVATE resilience GTest::gtest_main)
add_test(NAME AtomicLoadShedderTest COMMAND atomic_load_shedder_test)

add_executable(load_shedder_policy_test load_shedder_policy_test.cpp)
target_link_libraries(load_shedder_policy_test PRIVATE resilience GTest::gtest_main)
add_test(NAME LoadShedderPolicyTest COMMAND load_shedder_policy_test)
