project( tempus )
cmake_minimum_required(VERSION 2.6)

set( CMAKE_BUILD_TYPE RELWITHDEBINFO )

find_package(Boost)
if (NOT Boost_FOUND)
	set( Boost_INCLUDE_DIR "" CACHE PATH "Boost include directory" )
	set( Boost_LIBRARY_DIRS "" CACHE PATH "Boost library directory" )
endif (NOT Boost_FOUND)
add_definitions( "-DHAS_BOOST" )

if (WIN32)
	set( PQ_INCLUDE_DIR "C:/Program Files/PostgreSQL/9.1/include" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "C:/Program Files/PostgreSQL/9.1/lib" CACHE PATH "libpq lib directory" )
	set( PQ_LIBS libpq )
	# Disable verbose warnings
	add_definitions( "/D_CRT_SECURE_NO_WARNINGS /wd4290" )

	set( LIBTEMPUS tempus )
else (WIN32)
	set( PQ_INCLUDE_DIR "/usr/include/postgresql" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "/usr/lib" CACHE PATH "libpq lib directory" )
	set( PQ_LIBS pq dl boost_program_options )
	set (LIBTEMPUS tempus )
endif (WIN32)

include_directories( ${PQ_INCLUDE_DIR} ${Boost_INCLUDE_DIR} )
link_directories( ${PQ_LIBS_DIR} ${Boost_LIBRARY_DIRS} )

set( BUILD_TESTS OFF CACHE BOOL "Build unit tests ?" )
set( BUILD_WPS OFF CACHE BOOL "Build WPS server ?" )
set( BUILD_QGIS_PLUGIN OFF CACHE BOOL "Build QGIS plugin ?" )

add_subdirectory( core )

if(BUILD_TESTS)
	add_subdirectory( tests )
endif(BUILD_TESTS)

if(BUILD_WPS)
	add_subdirectory( wps )
endif(BUILD_WPS)

if(BUILD_QGIS_PLUGIN)
	add_subdirectory( wps_qgis )
endif(BUILD_QGIS_PLUGIN)