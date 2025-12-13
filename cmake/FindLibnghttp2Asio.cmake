# Find nghttp2-asio library (pre-installed in Docker image)
#
# This module defines:
#   Libnghttp2Asio_FOUND - True if nghttp2-asio was found
#   Libnghttp2Asio_INCLUDE_DIRS - Include directories
#   Libnghttp2Asio_LIBRARIES - Libraries to link
#   nghttp2_asio - Imported target

find_path(Libnghttp2Asio_INCLUDE_DIR
    NAMES nghttp2/asio_http2_server.h
    PATHS /usr/local/include /usr/include
)

find_library(Libnghttp2Asio_LIBRARY
    NAMES nghttp2_asio
    PATHS /usr/local/lib /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libnghttp2Asio
    REQUIRED_VARS Libnghttp2Asio_LIBRARY Libnghttp2Asio_INCLUDE_DIR
)

if(Libnghttp2Asio_FOUND)
    set(Libnghttp2Asio_INCLUDE_DIRS ${Libnghttp2Asio_INCLUDE_DIR})
    set(Libnghttp2Asio_LIBRARIES ${Libnghttp2Asio_LIBRARY})
    
    if(NOT TARGET nghttp2_asio)
        add_library(nghttp2_asio UNKNOWN IMPORTED)
        set_target_properties(nghttp2_asio PROPERTIES
            IMPORTED_LOCATION "${Libnghttp2Asio_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Libnghttp2Asio_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(Libnghttp2Asio_INCLUDE_DIR Libnghttp2Asio_LIBRARY)
