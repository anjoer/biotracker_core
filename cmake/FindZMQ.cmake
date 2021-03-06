#- Find ZeroMQ includes and library
#
# This module defines
#  ZMQ_INCLUDE_DIR
#  ZMQ_LIBRARIES, the libraries to link against to use ZMQ.
#  ZMQ_FOUND, If false, do not try to use ZMQ
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
 
IF (ZMQ_LIBRARIES AND ZMQ_INCLUDE_DIR)
    SET(ZMQ_FIND_QUIETLY TRUE) # Already in cache, be silent
ENDIF (ZMQ_LIBRARIES AND ZMQ_INCLUDE_DIR)
 
FIND_PATH(ZMQ_INCLUDE_DIR zmq.h zmq.hpp
    $ENV{ZMQ_DIR}/include
    $ENV{ZMQ_INCLUDE_DIR}
    /usr/include
    /usr/local/include
    /usr/local/include/zmq
)

# We need different handling of CMAKE include paths depending on whether we are using MSVC,
# because the library name is very non-standard in that case (and thus not found by a simple FIND_LIBRARY).
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    foreach (directory
        $ENV{ZMQ_DIR}/lib
        $ENV{ZMQ_LIBRARY_DIR}
        )
        file(GLOB zmq ${directory}/libzmq-v*.lib)
        IF (zmq AND NOT ZMQ_LIBRARY)
            SET(ZMQ_LIBRARY ${zmq})
        ENDIF (zmq AND NOT ZMQ_LIBRARY)
    endforeach(directory)
endif()
# If we are either on non-windows OR the custom windows search failed, try the normal one.
if (NOT ZMQ_LIBRARY)
    FIND_LIBRARY(ZMQ_LIBRARY NAMES zmq libzmq PATHS
        $ENV{ZMQ_DIR}/lib
        $ENV{ZMQ_LIBRARY_DIR}
        /usr/lib
        /usr/local/lib
        /usr/local/lib/zmq
        /usr/lib/x86_64-linux-gnu
    )
endif()

# handle the QUIETLY and REQUIRED arguments and set ZMQ_FOUND to TRUE
# if all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args ( ZMQ DEFAULT_MSG ZMQ_LIBRARY ZMQ_INCLUDE_DIR )

IF (ZMQ_FOUND)
    SET(ZMQ_LIBRARY ${ZMQ_LIBRARY} CACHE STRING "ZMQ library path." FORCE)
    SET(ZMQ_LIBRARIES ${ZMQ_LIBRARY})
    SET(ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR})
    MESSAGE(STATUS "Found these zmq libs: ${ZMQ_LIBRARIES}")
ENDIF (ZMQ_FOUND)
 
# Report the results.
IF (NOT ZMQ_FOUND)
        SET(ZMQ_DIR_MESSAGE "ZeroMQ was not found. Make sure ZMQ_LIBRARY and ZMQ_INCLUDE_DIR are set.")
        IF (NOT ZMQ_FIND_QUIETLY)
                MESSAGE(STATUS "${ZMQ_DIR_MESSAGE}")
        ELSE (NOT ZMQ_FIND_QUIETLY)
                IF (ZMQ_FIND_REQUIRED)
                        MESSAGE(FATAL_ERROR "${ZMQ_DIR_MESSAGE}")
                ENDIF (ZMQ_FIND_REQUIRED)
        ENDIF (NOT ZMQ_FIND_QUIETLY)
ENDIF (NOT ZMQ_FOUND)
 
 
 MARK_AS_ADVANCED(
    ZMQ_INCLUDE_DIRS
    ZMQ_LIBRARIES
 )