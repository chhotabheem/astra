cmake_minimum_required(VERSION 3.15)
project(hfsm)

# FetchContent removed in favor of manual download due to container git issues
# include(FetchContent)
# ...

add_library(hfsm INTERFACE)
target_include_directories(hfsm INTERFACE include)

enable_testing()
add_executable(test_hfsm_unit tests/test_hfsm_unit.cpp)
target_link_libraries(test_hfsm_unit PRIVATE hfsm)
add_test(NAME HfsmUnitTests COMMAND test_hfsm_unit)
