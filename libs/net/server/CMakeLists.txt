add_executable(astra_server main.cpp)

target_link_libraries(astra_server
    PRIVATE
        httpserver
        logger
        mongoclient
)
