add_library(astra_router src/Router.cpp)
target_include_directories(astra_router PUBLIC include)
target_link_libraries(astra_router PRIVATE astra_sanitizers astra_utils)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
