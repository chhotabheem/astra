find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# Generate protobuf code
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS config/execution.proto)

add_library(astra_execution
    src/MessageQueue.cpp
    src/AffinityExecutor.cpp
    src/PoolExecutor.cpp
    src/ObservableExecutor.cpp
    ${PROTO_SRCS}
)

target_include_directories(astra_execution
    PUBLIC
        include
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(astra_execution
    PRIVATE
        astra_sanitizers
        Threads::Threads
    PUBLIC
        observability
        protobuf::libprotobuf
)

# Add tests if testing is enabled
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
