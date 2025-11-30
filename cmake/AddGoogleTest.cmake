include(FetchContent)

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)

FetchContent_MakeAvailable(googletest)

# Enable GoogleTest module for gtest_discover_tests
include(GoogleTest)
