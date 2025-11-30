# Macro to add a standard Astra GTest executable
# Usage:
# astra_add_test(
#     TARGET <target_name>
#     SOURCES <source_files>...
#     [LIBRARIES <libs>...]
#     [INCLUDE_DIRS <dirs>...]
# )
macro(astra_add_test)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES LIBRARIES INCLUDE_DIRS)
    cmake_parse_arguments(ASTRA_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ASTRA_TEST_TARGET)
        message(FATAL_ERROR "astra_add_test: TARGET argument is required")
    endif()

    if(NOT ASTRA_TEST_SOURCES)
        message(FATAL_ERROR "astra_add_test: SOURCES argument is required")
    endif()

    add_executable(${ASTRA_TEST_TARGET} ${ASTRA_TEST_SOURCES})

    if(ASTRA_TEST_INCLUDE_DIRS)
        target_include_directories(${ASTRA_TEST_TARGET} PRIVATE ${ASTRA_TEST_INCLUDE_DIRS})
    endif()

    target_link_libraries(${ASTRA_TEST_TARGET}
        PRIVATE
            ${ASTRA_TEST_LIBRARIES}
            GTest::gtest_main
            GTest::gmock
    )

    # Enable test discovery
    include(GoogleTest)
    gtest_discover_tests(${ASTRA_TEST_TARGET})
endmacro()
