cmake_minimum_required(VERSION 3.25)

project(astra_execution)

find_package(Threads REQUIRED)

add_library(astra_execution
    src/ThreadPool.cpp
    src/StripedMessagePool.cpp
    src/ObservableMessagePool.cpp
)

target_include_directories(astra_execution
    PUBLIC
        include
)

target_link_libraries(astra_execution
    PUBLIC
        Threads::Threads
        observability
)

# Add tests if testing is enabled
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
