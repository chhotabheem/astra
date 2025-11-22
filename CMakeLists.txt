cmake_minimum_required(VERSION 3.15)
project(Astra)

enable_testing()
add_subdirectory(logger)

# Add the MongoDB client subdirectory
add_subdirectory(mongoclient)
