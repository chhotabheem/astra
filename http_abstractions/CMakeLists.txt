cmake_minimum_required(VERSION 3.15)
project(http_abstractions)

add_library(http_abstractions INTERFACE)
target_include_directories(http_abstractions INTERFACE include)
