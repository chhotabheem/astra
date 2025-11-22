cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

message(VERBOSE "Executing download step for mongo-c-driver")

block(SCOPE_FOR VARIABLES)

include("/app/astra/mongoclient/build/CMakeFiles/fc-tmp/mongo-c-driver/mongo-c-driver-gitclone.cmake")

endblock()
