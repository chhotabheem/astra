cmake_minimum_required(VERSION 3.25)

add_library(config INTERFACE)
target_include_directories(config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)



add_library(config_impl STATIC 
    src/ConfigValidator.cpp
    src/filesource/FileConfigSource.cpp
    src/parsers/JsonConfigParser.cpp
    src/ConfigProvider.cpp
)
target_include_directories(config_impl PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(config_impl 
    PUBLIC 
        observability
        astra_concurrency
        outcome
    PRIVATE
        json
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
