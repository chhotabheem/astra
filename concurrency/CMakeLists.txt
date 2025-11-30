cmake_minimum_required(VERSION 3.25)

project(astra_concurrency)

find_package(Threads REQUIRED)

add_library(astra_concurrency
    src/WorkerPool.cpp
    src/IoWorkerPool.cpp
)

target_include_directories(astra_concurrency
    PUBLIC
        include
)

target_link_libraries(astra_concurrency
    PUBLIC
        Threads::Threads
)

# Add tests if testing is enabled
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
