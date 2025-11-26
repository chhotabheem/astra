cmake_minimum_required(VERSION 3.15)
project(router)

add_library(router src/Router.cpp)
target_include_directories(router PUBLIC include)
target_link_libraries(router PUBLIC http_abstractions)
