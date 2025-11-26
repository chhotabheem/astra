cmake_minimum_required(VERSION 3.15)
project(http1.1client)

find_package(Boost REQUIRED COMPONENTS system thread)

add_library(http1.1client 
    src/Http1Client.cpp
)

target_include_directories(http1.1client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

target_link_libraries(http1.1client
    PUBLIC
        Boost::system
        Boost::thread
)

target_compile_features(http1.1client PUBLIC cxx_std_17)

enable_testing()
add_executable(test_http1.1client_unit tests/test_http1.1client_unit.cpp)
target_link_libraries(test_http1.1client_unit PRIVATE http1.1client)
add_test(NAME Http1.1ClientUnitTests COMMAND test_http1.1client_unit)
