# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/app/astra/build/_deps/mongo_cxx_driver-src")
  file(MAKE_DIRECTORY "/app/astra/build/_deps/mongo_cxx_driver-src")
endif()
file(MAKE_DIRECTORY
  "/app/astra/build/_deps/mongo_cxx_driver-build"
  "/app/astra/build/_deps/mongo_cxx_driver-subbuild/mongo_cxx_driver-populate-prefix"
  "/app/astra/build/_deps/mongo_cxx_driver-subbuild/mongo_cxx_driver-populate-prefix/tmp"
  "/app/astra/build/_deps/mongo_cxx_driver-subbuild/mongo_cxx_driver-populate-prefix/src/mongo_cxx_driver-populate-stamp"
  "/app/astra/build/_deps/mongo_cxx_driver-subbuild/mongo_cxx_driver-populate-prefix/src"
  "/app/astra/build/_deps/mongo_cxx_driver-subbuild/mongo_cxx_driver-populate-prefix/src/mongo_cxx_driver-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/app/astra/build/_deps/mongo_cxx_driver-subbuild/mongo_cxx_driver-populate-prefix/src/mongo_cxx_driver-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/app/astra/build/_deps/mongo_cxx_driver-subbuild/mongo_cxx_driver-populate-prefix/src/mongo_cxx_driver-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
