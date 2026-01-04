find_package(Protobuf REQUIRED)

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
    PUBLIC
        observability
        protobuf::libprotobuf
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
