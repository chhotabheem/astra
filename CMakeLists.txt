cmake_minimum_required(VERSION 3.15)
project(Astra)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug/Release/RelWithDebInfo/MinSizeRel)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

enable_testing()
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
