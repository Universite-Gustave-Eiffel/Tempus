# - Try to find fcgi
# Once done this will define
#
#  ICONV_FOUND - System has fcgi
#  ICONV_INCLUDE_DIR - The fcgi include directory
#  ICONV_LIBRARIES - The libraries needed to use fcgi
#
# You can define the environment variable ICONV_ROOT to specify custom install directory

#=============================================================================
# Copyright 2013-2009 Oslandia
#

find_path(ICONV_INCLUDE_DIR NAME iconv.h
   PATHS
   $ENV{ICONV_ROOT}
   /usr/local/include
   /usr/include
   C:/
   )

find_library(ICONV_LIBRARIES NAME iconv
   PATHS
   $ENV{ICONV_ROOT}
   /usr/local/lib
   /usr/lib
   C:/
   )

if (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
    set(ICONV_FOUND 1)
    message( STATUS "found iconv" )
else()
    message( FATAL_ERROR "iconv not found" )
endif()
