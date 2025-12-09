# Sanitizer Configuration
# Supports: Address (ASan), Thread (TSan), Memory (MSan), Undefined (UBSan)
# Works with: Clang and GCC (MSan is Clang-only)
#
# USAGE: Link your Astra libraries to astra_sanitizers PRIVATE:
#   target_link_libraries(my_library PRIVATE astra_sanitizers)
#
# This approach prevents third-party dependencies from inheriting sanitizer flags.

set(SANITIZE_MODE "none" CACHE STRING "Sanitizer mode: address, thread, memory, undefined, none")
set_property(CACHE SANITIZE_MODE PROPERTY STRINGS "address" "thread" "memory" "undefined" "none")

if(SANITIZE_MODE STREQUAL "none")
    message(STATUS "Sanitizers: Disabled")
    # Create empty interface library so targets can always link to it
    add_library(astra_sanitizers INTERFACE)
    return()
endif()

message(STATUS "Sanitizer Mode: ${SANITIZE_MODE}")

# Create interface library for sanitizer flags
add_library(astra_sanitizers INTERFACE)

if(SANITIZE_MODE STREQUAL "address")
    # AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan)
    # Detects: Out-of-bounds, Use-after-free, Leaks, Integer overflow, etc.
    target_compile_options(astra_sanitizers INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
    target_link_options(astra_sanitizers INTERFACE -fsanitize=address,undefined)

elseif(SANITIZE_MODE STREQUAL "thread")
    # ThreadSanitizer (TSan)
    # Detects: Data races, Deadlocks
    target_compile_options(astra_sanitizers INTERFACE -fsanitize=thread -g)
    target_link_options(astra_sanitizers INTERFACE -fsanitize=thread)

elseif(SANITIZE_MODE STREQUAL "memory")
    # MemorySanitizer (MSan)
    # Detects: Use of uninitialized memory
    # Note: Requires Clang and instrumented libc++
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(astra_sanitizers INTERFACE -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -g -stdlib=libc++)
        target_link_options(astra_sanitizers INTERFACE -fsanitize=memory -stdlib=libc++ -lc++abi)
    else()
        message(FATAL_ERROR "MemorySanitizer (MSan) requires Clang. You are using ${CMAKE_CXX_COMPILER_ID}.")
    endif()

elseif(SANITIZE_MODE STREQUAL "undefined")
    # UndefinedBehaviorSanitizer (UBSan) Standalone
    target_compile_options(astra_sanitizers INTERFACE -fsanitize=undefined -g)
    target_link_options(astra_sanitizers INTERFACE -fsanitize=undefined)

else()
    message(FATAL_ERROR "Unknown SANITIZE_MODE: ${SANITIZE_MODE}")
endif()
