# - Try to find fcgi
# Once done this will define
#
#  GLIB_FOUND - System has fcgi
#  GLIB_INCLUDE_DIR - The fcgi include directory
#  GLIB_LIBRARIES - The libraries needed to use fcgi
#
# You can define the environment variable GLIB_ROOT to specify custom install directory

#=============================================================================
# Copyright 2013-2009 Oslandia
#

find_path(GLIB_INCLUDE_DIR NAME glib.h
   PATHS
   $ENV{GLIB_ROOT}/include
   PATH_SUFFIXES glib-2.0
   )

if (UNIX)
    find_path(GLIB_INCLUDE_CONFIG_DIR NAME glibconfig.h
        PATHS
        /usr/lib/x86_64-linux-gnu/glib-2.0/include
        )
endif()


find_library(GLIB_LIBRARIES NAMES glib-2.0
   PATHS
   $ENV{GLIB_ROOT}/lib
   )


if (GLIB_INCLUDE_DIR AND GLIB_LIBRARIES AND ((NOT UNIX) OR GLIB_INCLUDE_CONFIG_DIR))
    set(GLIB_INCLUDE_DIR ${GLIB_INCLUDE_DIR} ${GLIB_INCLUDE_CONFIG_DIR})
    set(GLIB_FOUND 1)
    message( STATUS "Found glib: " ${GLIB_LIBRARIES} )
else()
    if (GLIB_FIND_REQUIRED)
        message( FATAL_ERROR "glib not found" )
    else()
        message( WARNING "glib not found" )
    endif()
endif()
