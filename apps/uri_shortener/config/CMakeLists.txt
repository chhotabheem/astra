# Proto Config Generation
# Generates protobuf code for uri_shortener config with library imports

find_package(Protobuf REQUIRED)

# Import paths for library protos
set(PROTO_IMPORT_DIRS
    ${CMAKE_SOURCE_DIR}/libs/core/observability/config
    ${CMAKE_SOURCE_DIR}/libs/core/resilience/config
    ${CMAKE_SOURCE_DIR}/libs/core/execution/config
    ${CMAKE_SOURCE_DIR}/libs/net/http/v2/server/config
    ${CMAKE_SOURCE_DIR}/libs/net/http/v2/client/config
)

# Generate resilience proto (not yet handled by its lib)
protobuf_generate_cpp(RES_PROTO_SRCS RES_PROTO_HDRS
    ${CMAKE_SOURCE_DIR}/libs/core/resilience/config/resilience.proto)

# Generate app proto with imports
set(PROTOBUF_IMPORT_DIRS ${PROTO_IMPORT_DIRS})
protobuf_generate_cpp(APP_PROTO_SRCS APP_PROTO_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/uri_shortener.proto)

# Create library for generated protobuf code
add_library(uri_shortener_config STATIC
    ${APP_PROTO_SRCS} ${APP_PROTO_HDRS}
    ${RES_PROTO_SRCS} ${RES_PROTO_HDRS}
)

target_include_directories(uri_shortener_config 
    PUBLIC 
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(uri_shortener_config 
    PUBLIC 
        protobuf::libprotobuf
        observability
        astra_execution
        http2server
        http2client
)
target_compile_features(uri_shortener_config PUBLIC cxx_std_17)
