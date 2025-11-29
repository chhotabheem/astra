# Valgrind Verification Targets
# Defines: test_memcheck, test_helgrind, test_massif, test_callgrind, test_cachegrind, test_valgrind_all

find_program(MEMORYCHECK_COMMAND valgrind)

if(NOT MEMORYCHECK_COMMAND)
    message(WARNING "Valgrind not found. Valgrind targets will not be available.")
    return()
endif()

# Default options for standard CTest MemCheck
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --error-exitcode=1")

# 1. MemCheck (Memory Leaks)
add_custom_target(test_memcheck
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --output-on-failure
    COMMENT "Running MemCheck (Memory Leak Detection)"
)

# 2. Helgrind (Thread Safety)
add_custom_target(test_helgrind
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --overwrite "MemoryCheckCommandOptions=--tool=helgrind --error-exitcode=1 --suppressions=${CMAKE_SOURCE_DIR}/valgrind.supp" --output-on-failure
    COMMENT "Running Helgrind (Thread Error Detection)"
)

# 3. Massif (Heap Profiling)
add_custom_target(test_massif
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --overwrite "MemoryCheckCommandOptions=--tool=massif" --output-on-failure
    COMMENT "Running Massif (Heap Profiling)"
)

# 4. Callgrind (Call Graph)
add_custom_target(test_callgrind
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --overwrite "MemoryCheckCommandOptions=--tool=callgrind" --output-on-failure
    COMMENT "Running Callgrind (Call Graph Profiling)"
)

# 5. Cachegrind (Cache Usage)
add_custom_target(test_cachegrind
    COMMAND ${CMAKE_CTEST_COMMAND} -T MemCheck --overwrite "MemoryCheckCommandOptions=--tool=cachegrind" --output-on-failure
    COMMENT "Running Cachegrind (Cache Profiling)"
)

# 6. Unified Target (Run All)
add_custom_target(test_valgrind_all
    COMMAND ${CMAKE_COMMAND} --build . --target test_memcheck
    COMMAND ${CMAKE_COMMAND} --build . --target test_helgrind
    COMMAND ${CMAKE_COMMAND} --build . --target test_massif
    COMMAND ${CMAKE_COMMAND} --build . --target test_callgrind
    COMMAND ${CMAKE_COMMAND} --build . --target test_cachegrind
    COMMENT "Running ALL Valgrind verification tools..."
)
