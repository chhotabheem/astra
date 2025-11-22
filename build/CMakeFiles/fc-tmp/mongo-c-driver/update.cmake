cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

message(VERBOSE "Executing update step for mongo-c-driver")

block(SCOPE_FOR VARIABLES)

include("/app/astra/build/CMakeFiles/fc-tmp/mongo-c-driver/mongo-c-driver-gitupdate.cmake")

endblock()
