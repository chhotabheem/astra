cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# First build nghttp2-asio (shared dependency)
add_subdirectory(nghttp2)

# Then build server and client
add_subdirectory(server)
add_subdirectory(client)
