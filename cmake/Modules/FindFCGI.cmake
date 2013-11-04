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
   $ENV{FCGI_ROOT}
   /usr/local/include
   /usr/include
   C:/
   )

find_library(FCGI_LIBRARIES NAMES fcgi fcgi++ libfcgi 
   PATHS
   $ENV{FCGI_ROOT}
   /usr/local/lib
   /usr/lib
   C:/
   )

if (UNIX)
    find_library(FCGI_CPP_LIBRARIES NAMES fcgi++ 
        PATHS
        $ENV{FCGI_ROOT}
        /usr/local/lib
        /usr/lib
        )
else()
    set(FCGI_CPP_LIBRARIES "")
endif()

if (FCGI_INCLUDE_DIR AND FCGI_LIBRARIES AND FCGI_CPP_LIBRARIES)
    set(FCGI_LIBRARIES ${FCGI_LIBRARIES} ${FCGI_CPP_LIBRARIES})
    set(FCGI_FOUND 1)
    message( STATUS "found fcgi" )
else()
    if (REQUIRED)
        message( FATAL_ERROR "fcgi not found" )
    else()
        message( ERROR "fcgi not found" )
    endif()
endif()
