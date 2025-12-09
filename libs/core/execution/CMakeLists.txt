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
    PRIVATE
        astra_sanitizers
        Threads::Threads
    PUBLIC
        observability
)

# Add tests if testing is enabled
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
