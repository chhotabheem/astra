# Find hiredis library (system-installed via apt)
#
# This module defines:
#   hiredis_FOUND - True if hiredis was found
#   hiredis_INCLUDE_DIRS - Include directories
#   hiredis_LIBRARIES - Libraries to link
#   hiredis::hiredis - Imported target

find_path(hiredis_INCLUDE_DIR
    NAMES hiredis/hiredis.h
    PATHS /usr/include /usr/local/include
)

find_library(hiredis_LIBRARY
    NAMES hiredis
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hiredis
    REQUIRED_VARS hiredis_LIBRARY hiredis_INCLUDE_DIR
)

if(hiredis_FOUND)
    set(hiredis_INCLUDE_DIRS ${hiredis_INCLUDE_DIR})
    set(hiredis_LIBRARIES ${hiredis_LIBRARY})
    
    if(NOT TARGET hiredis::hiredis)
        add_library(hiredis::hiredis UNKNOWN IMPORTED)
        set_target_properties(hiredis::hiredis PROPERTIES
            IMPORTED_LOCATION "${hiredis_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${hiredis_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(hiredis_INCLUDE_DIR hiredis_LIBRARY)
