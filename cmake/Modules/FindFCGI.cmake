# - Try to find fcgi
# Once done this will define
#
#  FCGI_FOUND - System has fcgi
#  FCGI_INCLUDE_DIR - The fcgi include directory
#  FCGI_LIBRARIES - The libraries needed to use fcgi
#
# You can define the environment variable FCGI_ROOT to specify custom install directory

#=============================================================================
# Copyright 2013-2009 Oslandia
#

find_path(FCGI_INCLUDE_DIR NAME fcgio.h
   PATHS
   $ENV{FCGI_ROOT}/include
   )

find_library(FCGI_LIBRARIES NAMES fcgi fcgi++ libfcgi 
   PATHS
   $ENV{FCGI_ROOT}/libfcgi/Release
   )

if (UNIX)
    find_library(FCGI_CPP_LIBRARIES NAMES fcgi++ )
endif()

if (FCGI_INCLUDE_DIR AND FCGI_LIBRARIES AND ((NOT UNIX) OR FCGI_CPP_LIBRARIES))
    set(FCGI_LIBRARIES ${FCGI_LIBRARIES} ${FCGI_CPP_LIBRARIES})
    set(FCGI_FOUND 1)
    message( STATUS "Found fcgi: " ${FCGI_LIBRARIES} )
else()
    if (FCGI_FIND_REQUIRED)
        message( FATAL_ERROR "fcgi not found" )
    else()
        message( WARNING "fcgi not found" )
    endif()
endif()
