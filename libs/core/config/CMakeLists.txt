# Config library - now provides only file watching infrastructure
# Main configuration is handled by app-specific proto config (ProtoConfigLoader)

add_library(config INTERFACE)
target_include_directories(config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Implementation: FileConfigSource for file watching (reusable infrastructure)
add_library(config_impl STATIC 
    src/filesource/FileConfigSource.cpp
)
target_include_directories(config_impl PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(config_impl 
    PRIVATE
        astra_sanitizers
    PUBLIC 
        observability
        astra_execution
        outcome
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
