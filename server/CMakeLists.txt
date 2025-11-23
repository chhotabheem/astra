cmake_minimum_required(VERSION 3.15)
project(AstraServer)

add_executable(astra_server main.cpp)

target_link_libraries(astra_server
    PRIVATE
        httpserver
        logger
        mongoclient
)
