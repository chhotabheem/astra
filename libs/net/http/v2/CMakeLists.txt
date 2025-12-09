# First build nghttp2-asio (shared dependency)
add_subdirectory(nghttp2)

# Then build server and client
add_subdirectory(server)
add_subdirectory(client)
