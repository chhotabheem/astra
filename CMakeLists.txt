cmake_minimum_required(VERSION 3.15)
project(Astra)

enable_testing()
add_subdirectory(jsonformatter)
add_subdirectory(logger)
add_subdirectory(http2server)

# Add the MongoDB client subdirectory
add_subdirectory(mongoclient)

# Add the main server application
# add_subdirectory(server)
