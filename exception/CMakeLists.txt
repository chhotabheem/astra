cmake_minimum_required(VERSION 3.15)
project(exception)

add_library(exception src/AstraException.cpp)

target_include_directories(exception
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

target_compile_features(exception PUBLIC cxx_std_17)

enable_testing()
add_executable(test_exception_unit tests/test_exception_unit.cpp)
target_link_libraries(test_exception_unit PRIVATE exception)
add_test(NAME ExceptionUnitTests COMMAND test_exception_unit)
