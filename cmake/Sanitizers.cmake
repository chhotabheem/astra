# Sanitizer Configuration
# Supports: Address (ASan), Thread (TSan), Memory (MSan), Undefined (UBSan)
# Works with: Clang and GCC (MSan is Clang-only)

set(SANITIZE_MODE "none" CACHE STRING "Sanitizer mode: address, thread, memory, undefined, none")
set_property(CACHE SANITIZE_MODE PROPERTY STRINGS "address" "thread" "memory" "undefined" "none")

if(SANITIZE_MODE STREQUAL "none")
    message(STATUS "Sanitizers: Disabled")
    return()
endif()

message(STATUS "Sanitizer Mode: ${SANITIZE_MODE}")

if(SANITIZE_MODE STREQUAL "address")
    # AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan)
    # Detects: Out-of-bounds, Use-after-free, Leaks, Integer overflow, etc.
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)

elseif(SANITIZE_MODE STREQUAL "thread")
    # ThreadSanitizer (TSan)
    # Detects: Data races, Deadlocks
    add_compile_options(-fsanitize=thread -g)
    add_link_options(-fsanitize=thread)

elseif(SANITIZE_MODE STREQUAL "memory")
    # MemorySanitizer (MSan)
    # Detects: Use of uninitialized memory
    # Note: Requires Clang and instrumented libc++
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -g -stdlib=libc++)
        add_link_options(-fsanitize=memory -stdlib=libc++ -lc++abi)
    else()
        message(FATAL_ERROR "MemorySanitizer (MSan) requires Clang. You are using ${CMAKE_CXX_COMPILER_ID}.")
    endif()

elseif(SANITIZE_MODE STREQUAL "undefined")
    # UndefinedBehaviorSanitizer (UBSan) Standalone
    add_compile_options(-fsanitize=undefined -g)
    add_link_options(-fsanitize=undefined)

else()
    message(FATAL_ERROR "Unknown SANITIZE_MODE: ${SANITIZE_MODE}")
endif()
