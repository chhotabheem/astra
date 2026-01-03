# -------------------------------------------------------
# Code Coverage Configuration
# Supports: GCC (gcov) and Clang (llvm-cov)
# Generates: HTML reports via lcov/genhtml or llvm-cov
# -------------------------------------------------------

option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(NOT ENABLE_COVERAGE)
    return()
endif()

message(STATUS "Code Coverage: Enabled")

# Coverage requires Debug build for meaningful line mapping
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Coverage works best with Debug build. Current: ${CMAKE_BUILD_TYPE}")
endif()

# -------------------------------------------------------
# Add coverage compiler flags
# -------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # GCC: Equivalent to -fprofile-arcs -ftest-coverage
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
    message(STATUS "Coverage: Using GCC/gcov")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang Source-Based Coverage
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    message(STATUS "Coverage: Using Clang/llvm-cov")

else()
    message(FATAL_ERROR "Coverage not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# -------------------------------------------------------
# Find coverage tools
# -------------------------------------------------------
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)
find_program(LLVM_COV_PATH llvm-cov)
find_program(LLVM_PROFDATA_PATH llvm-profdata)

# -------------------------------------------------------
# GCC Coverage Report Target
# -------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND LCOV_PATH AND GENHTML_PATH)

    # GCC 15 compatibility workaround
    set(LCOV_IGNORE_ERRORS --ignore-errors inconsistent,mismatch,negative,unused)

    add_custom_target(coverage_report
        COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR} --output-file coverage.info ${LCOV_IGNORE_ERRORS}
        COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/tests/*' '*/_deps/*' --output-file coverage.info ${LCOV_IGNORE_ERRORS}
        COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage_html ${LCOV_IGNORE_ERRORS}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating GCC coverage report in ${CMAKE_BINARY_DIR}/coverage_html"
    )

    add_custom_target(coverage_clean
        COMMAND ${LCOV_PATH} --zerocounters --directory ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage_html
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/coverage.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Cleaning coverage data"
    )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(WARNING "lcov/genhtml not found. Install with: sudo apt install lcov")
endif()

# -------------------------------------------------------
# Clang Coverage Report Target
# -------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND LLVM_COV_PATH AND LLVM_PROFDATA_PATH)

    file(WRITE ${CMAKE_BINARY_DIR}/merge_profdata.sh
"#!/bin/bash
find ${CMAKE_BINARY_DIR} -name '*.profraw' | xargs ${LLVM_PROFDATA_PATH} merge -sparse -o ${CMAKE_BINARY_DIR}/coverage.profdata
"
    )

    file(WRITE ${CMAKE_BINARY_DIR}/gen_coverage.sh
"#!/bin/bash
TEST_BINS=$(find ${CMAKE_BINARY_DIR}/bin -type f -executable \\( -name \"*_test\" -o -name \"*_gtest\" \\) 2>/dev/null | head -20)

if [ -z \"$TEST_BINS\" ]; then
  echo \"No test executables found in ${CMAKE_BINARY_DIR}/bin\"
  exit 1
fi

OBJECT_FLAGS=\"\"
for BIN in $TEST_BINS; do
  OBJECT_FLAGS=\"$OBJECT_FLAGS -object=$BIN\"
done

FIRST_BIN=$(echo \"$TEST_BINS\" | head -1)

echo \"Generating coverage for:\"
echo \"$TEST_BINS\"

${LLVM_COV_PATH} show $FIRST_BIN $OBJECT_FLAGS \
  -instr-profile=${CMAKE_BINARY_DIR}/coverage.profdata \
  --format=html \
  --output-dir=${CMAKE_BINARY_DIR}/coverage_html \
  --ignore-filename-regex='(/usr/|_deps/|/test/)'

${LLVM_COV_PATH} report $FIRST_BIN $OBJECT_FLAGS \
  -instr-profile=${CMAKE_BINARY_DIR}/coverage.profdata \
  --ignore-filename-regex='(/usr/|_deps/|/test/)'
"
    )

    add_custom_target(coverage_report
        COMMAND ${CMAKE_COMMAND} -E echo "Merging profile data..."
        COMMAND bash ${CMAKE_BINARY_DIR}/merge_profdata.sh
        COMMAND ${CMAKE_COMMAND} -E echo "Generating coverage report..."
        COMMAND bash ${CMAKE_BINARY_DIR}/gen_coverage.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating Clang coverage report in ${CMAKE_BINARY_DIR}/coverage_html"
    )

    add_custom_target(coverage_clean
        COMMAND bash -c "find ${CMAKE_BINARY_DIR} -name '*.profraw' -delete 2>/dev/null || true"
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/coverage.profdata
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage_html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Cleaning Clang coverage data"
    )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "llvm-cov/llvm-profdata not found. Install with: sudo apt install llvm")
endif()
