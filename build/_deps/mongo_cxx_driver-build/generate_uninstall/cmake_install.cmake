# Install script for directory: /app/astra/build/_deps/mongo_cxx_driver-src/generate_uninstall

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  
      string (REPLACE ";" "\n" MONGOCXX_INSTALL_MANIFEST_CONTENT
         "${CMAKE_INSTALL_MANIFEST_FILES}")
      file (WRITE "mongocxx_install_manifest.txt"
         "${MONGOCXX_INSTALL_MANIFEST_CONTENT}")
      execute_process (
         COMMAND
            find "$ENV{DESTDIR}//usr/local/include/bsoncxx/v_noabi" "$ENV{DESTDIR}//usr/local/include/mongocxx/v_noabi" -type d -empty -delete
      )
      execute_process (
         COMMAND
            /usr/bin/cmake -E env
            "/app/astra/build/_deps/mongo_cxx_driver-src/etc/generate-uninstall.sh"
            mongocxx_install_manifest.txt
            /usr/local
         OUTPUT_FILE
            "/app/astra/build/_deps/mongo_cxx_driver-build/generate_uninstall/uninstall.sh"
      )

      # Ensure generated uninstall script has executable permissions.
      if ("3.31.6" VERSION_GREATER_EQUAL "3.19.0")
         file (
            CHMOD "/app/astra/build/_deps/mongo_cxx_driver-build/generate_uninstall/uninstall.sh"
            PERMISSIONS
                  OWNER_READ OWNER_WRITE OWNER_EXECUTE
                  GROUP_READ GROUP_EXECUTE
                  WORLD_READ WORLD_EXECUTE
         )
      else ()
         # Workaround lack of file(CHMOD).
         file (
            COPY "/app/astra/build/_deps/mongo_cxx_driver-build/generate_uninstall/uninstall.sh"
            DESTINATION "uninstall.sh.d"
            FILE_PERMISSIONS
                  OWNER_READ OWNER_WRITE OWNER_EXECUTE
                  GROUP_READ GROUP_EXECUTE
                  WORLD_READ WORLD_EXECUTE
         )
         file (RENAME "uninstall.sh.d/uninstall.sh" "uninstall.sh")
      endif ()
   
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/mongo-cxx-driver" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/app/astra/build/_deps/mongo_cxx_driver-build/generate_uninstall/uninstall.sh")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/app/astra/build/_deps/mongo_cxx_driver-build/generate_uninstall/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
