cmake_minimum_required(VERSION 3.15)
project(Astra)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug/Release/RelWithDebInfo/MinSizeRel)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Enable AddressSanitizer (ASan)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

# Enable CTest and Valgrind
include(CTest)
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --error-exitcode=1")
add_subdirectory(jsonformatter)
add_subdirectory(logger)
add_subdirectory(exception)
add_subdirectory(hfsm)
add_subdirectory(boost_msm)
add_subdirectory(redisclient)
add_subdirectory(prometheus_client)
add_subdirectory(zookeeperclient)
add_subdirectory(http1.1)
add_subdirectory(http_abstractions)
add_subdirectory(router)
add_subdirectory(http2)

add_subdirectory(mongoclient)

# Add the main server application
# add_subdirectory(server)
