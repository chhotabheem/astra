find_package(Boost REQUIRED COMPONENTS system thread)

add_library(http1.1client 
    src/Http1Client.cpp
)

target_include_directories(http1.1client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

target_link_libraries(http1.1client
    PRIVATE
        astra_sanitizers
        Boost::system
        Boost::thread
)

target_compile_features(http1.1client PUBLIC cxx_std_17)

enable_testing()

astra_add_test(
    TARGET http1_client_test
    SOURCES tests/http1_client_test.cpp
    LIBRARIES http1.1client
)
