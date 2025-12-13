add_library(resilience
    src/AtomicLoadShedder.cpp
    src/LoadShedderPolicy.cpp
)

target_include_directories(resilience
    PUBLIC
        include
)

target_link_libraries(resilience
    PRIVATE
        astra_sanitizers
    PUBLIC
        astra_execution
)

# Add tests if testing is enabled
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
