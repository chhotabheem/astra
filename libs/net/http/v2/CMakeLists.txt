# nghttp2-asio is pre-installed in Docker image
find_package(Libnghttp2Asio REQUIRED)

# Build server and client
add_subdirectory(server)
add_subdirectory(client)
