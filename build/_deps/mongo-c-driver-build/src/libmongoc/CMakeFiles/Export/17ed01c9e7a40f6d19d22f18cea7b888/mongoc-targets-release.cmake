#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mongoc::static" for configuration "Release"
set_property(TARGET mongoc::static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(mongoc::static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmongoc2.a"
  )

list(APPEND _cmake_import_check_targets mongoc::static )
list(APPEND _cmake_import_check_files_for_mongoc::static "${_IMPORT_PREFIX}/lib/libmongoc2.a" )

# Import target "mongoc::shared" for configuration "Release"
set_property(TARGET mongoc::shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(mongoc::shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmongoc2.so.2.1.2"
  IMPORTED_SONAME_RELEASE "libmongoc2.so.2"
  )

list(APPEND _cmake_import_check_targets mongoc::shared )
list(APPEND _cmake_import_check_files_for_mongoc::shared "${_IMPORT_PREFIX}/lib/libmongoc2.so.2.1.2" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
