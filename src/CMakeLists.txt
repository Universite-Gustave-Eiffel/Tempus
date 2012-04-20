project( tempus )
cmake_minimum_required(VERSION 2.6)

set( CMAKE_BUILD_TYPE RELWITHDEBINFO )

find_package(Boost)
if (NOT Boost_FOUND)
	set( Boost_INCLUDE_DIR "" CACHE PATH "Boost include directory" )
	set( Boost_LIBRARY_DIRS "" CACHE PATH "Boost library directory" )
endif (NOT Boost_FOUND)
add_definitions( "-DHAS_BOOST" )

set( BUILD_TESTS OFF CACHE BOOL "Build unit tests ?" )
set( BUILD_WPS OFF CACHE BOOL "Build WPS server ?" )

add_subdirectory( core )

if(BUILD_TESTS)
	add_subdirectory( tests )
endif(BUILD_TESTS)

if(BUILD_WPS)
	add_subdirectory( wps )
endif(BUILD_WPS)