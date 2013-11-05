# - Try to find shp lib
# Once done this will define
#
#  SHP_FOUND - System has shp lib
#  SHP_INCLUDE_DIR - The shp include directory
#  SHP_LIBRARIES - The libraries needed to use shp
#
# You can define the environment variable SHP_ROOT to specify custom install directory

#=============================================================================
# Copyright 2013-2009 Oslandia
#

find_path(SHP_INCLUDE_DIR NAME shapefil.h
   PATHS
   $ENV{SHP_ROOT}/include
   )

find_library(SHP_LIBRARIES NAMES shp shapelib_i
   PATHS
   $ENV{SHP_ROOT}/lib
   )

if (SHP_INCLUDE_DIR AND SHP_LIBRARIES)
    set(SHP_FOUND 1)
    message( STATUS "Found shp: " ${SHP_LIBRARIES} )
else()
    if (SHP_FIND_REQUIRED)
        message( FATAL_ERROR "shp not found" )
    else()
        message( WARNING "shp not found" )
    endif()
endif()
