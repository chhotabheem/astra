cmake_minimum_required(VERSION 3.25)
project(router)

add_library(router src/Router.cpp)
target_include_directories(router PUBLIC include)

