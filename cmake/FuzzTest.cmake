# FuzzTest.cmake - Google FuzzTest integration via FetchContent
# Only active when ENABLE_FUZZTEST=ON (set by clang-unit-fuzz/clang-deep-fuzz presets)

option(ENABLE_FUZZTEST "Enable FuzzTest for fuzz testing" OFF)

if(ENABLE_FUZZTEST)
    message(STATUS "FuzzTest: Enabled - downloading via FetchContent...")
    
    include(FetchContent)
    FetchContent_Declare(
        fuzztest
        GIT_REPOSITORY https://github.com/google/fuzztest.git
        GIT_TAG        2024-10-28
        GIT_SHALLOW    TRUE
    )
    
    # Disable ANTLR's test suite - it has a post-build step that copies
    # libraries to ${CMAKE_HOME_DIRECTORY}/dist (our source root)
    set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(fuzztest)
    
    # Setup fuzzing flags if in deep fuzzing mode
    if(FUZZTEST_FUZZING_MODE)
        fuzztest_setup_fuzzing_flags()
        message(STATUS "FuzzTest: Deep fuzzing mode (coverage instrumentation enabled)")
    else()
        message(STATUS "FuzzTest: Unit test mode (quick runs)")
    endif()
else()
    message(STATUS "FuzzTest: Disabled")
endif()
