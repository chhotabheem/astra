cmake_minimum_required(VERSION 3.15)
project(Astra)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug/Release/RelWithDebInfo/MinSizeRel)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Enable AddressSanitizer (ASan)
# Sanitizer Configuration
set(SANITIZE_MODE "none" CACHE STRING "Sanitizer mode: address, thread, memory, undefined, none")
set_property(CACHE SANITIZE_MODE PROPERTY STRINGS "address" "thread" "memory" "undefined" "none")

if(SANITIZE_MODE STREQUAL "address")
    message(STATUS "Sanitizer Mode: Address (ASan + LSan + UBSan)")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
elseif(SANITIZE_MODE STREQUAL "thread")
    message(STATUS "Sanitizer Mode: Thread (TSan + UBSan)")
    add_compile_options(-fsanitize=thread,undefined -g)
    add_link_options(-fsanitize=thread,undefined)
elseif(SANITIZE_MODE STREQUAL "memory")
    message(STATUS "Sanitizer Mode: Memory (MSan)")
    # MSan requires instrumented libc++, so we link against it explicitly
    add_compile_options(-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -g -stdlib=libc++)
    add_link_options(-fsanitize=memory -stdlib=libc++ -lc++abi)
elseif(SANITIZE_MODE STREQUAL "undefined")
    message(STATUS "Sanitizer Mode: Undefined (UBSan)")
    add_compile_options(-fsanitize=undefined -g)
    add_link_options(-fsanitize=undefined)
endif()

# Enable CTest and Valgrind
include(CTest)
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --error-exitcode=1")

# Custom targets for Valgrind tools
add_custom_target(test_memcheck
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --output-on-failure
    COMMENT "Running MemCheck (Memory Leak Detection)"
)

add_custom_target(test_helgrind
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --overwrite "MemoryCheckCommandOptions=--tool=helgrind --error-exitcode=1 --suppressions=${CMAKE_SOURCE_DIR}/valgrind.supp" --output-on-failure
    COMMENT "Running Helgrind (Thread Error Detection)"
)

add_custom_target(test_cachegrind
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --overwrite "MemoryCheckCommandOptions=--tool=cachegrind" --output-on-failure
    COMMENT "Running Cachegrind (Cache Profiling)"
)

add_custom_target(test_callgrind
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --overwrite "MemoryCheckCommandOptions=--tool=callgrind" --output-on-failure
    COMMENT "Running Callgrind (Call Graph Profiling)"
)

add_custom_target(test_massif
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --overwrite "MemoryCheckCommandOptions=--tool=massif" --output-on-failure
    COMMENT "Running Massif (Heap Profiling)"
)
add_subdirectory(jsonformatter)
add_subdirectory(logger)
add_subdirectory(exception)
add_subdirectory(hfsm)
add_subdirectory(boost_msm)
add_subdirectory(redisclient)
add_subdirectory(prometheus_client)
add_subdirectory(zookeeperclient)
add_subdirectory(http1.1)

add_subdirectory(router)
add_subdirectory(http2)

add_subdirectory(mongoclient)

# Add the main server application
# add_subdirectory(server)
