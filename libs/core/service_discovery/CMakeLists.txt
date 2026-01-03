# Service Discovery Library

add_library(service_discovery STATIC
    src/StaticServiceResolver.cpp
)

target_include_directories(service_discovery
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(service_discovery
    PRIVATE
        astra_sanitizers
)

target_compile_features(service_discovery PUBLIC cxx_std_17)

# Tests
if(BUILD_TESTING)
    include(GoogleTest)
    
    astra_add_test(
        TARGET test_service_discovery
        SOURCES tests/service_discovery_test.cpp
        LIBRARIES service_discovery
    )
endif()
