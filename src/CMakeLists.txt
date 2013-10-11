set( CMAKE_BUILD_TYPE RELWITHDEBINFO )

find_package(Boost REQUIRED COMPONENTS timer system)
if (NOT Boost_FOUND)
	set( Boost_INCLUDE_DIR "" CACHE PATH "Boost include directory" )
	set( Boost_LIBRARY_DIRS "" CACHE PATH "Boost library directory" )
endif (NOT Boost_FOUND)
add_definitions( "-DHAS_BOOST" )

if (WIN32)
	set( PQ_INCLUDE_DIR "C:/Program Files(x86)/PostgreSQL/9.3/include" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "C:/Program Files (x86)/PostgreSQL/9.3/lib" CACHE PATH "libpq lib directory" )
	set( PQ_LIBS libpq )

	set( LIBTEMPUS tempus )
else (WIN32)
	set( PQ_INCLUDE_DIR "/usr/include/postgresql" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "/usr/lib" CACHE PATH "libpq lib directory" )
	set( PQ_LIBS pq dl boost_program_options )
	set (LIBTEMPUS tempus )
endif (WIN32)

include_directories( ${PQ_INCLUDE_DIR} ${Boost_INCLUDE_DIR} )
link_directories( ${PQ_LIBS_DIR} ${Boost_LIBRARY_DIRS} )

set( BUILD_CORE ON CACHE BOOL "Build Tempus core ?" )
set( BUILD_TESTS OFF CACHE BOOL "Build unit tests ?" )
set( BUILD_WPS OFF CACHE BOOL "Build WPS server ?" )
set( BUILD_QGIS_PLUGIN OFF CACHE BOOL "Build QGIS plugin ?" )
set( BUILD_INTRALYS_PLUGIN OFF CACHE BOOL "Build Intralys plugin ?" )
set( BUILD_TEMPUS_PLUGINS ON CACHE BOOL "Build Tempus plugins ?" )

if(BUILD_CORE)
  add_subdirectory( core )
endif(BUILD_CORE)

if(BUILD_TESTS)
  add_subdirectory( tests )
endif(BUILD_TESTS)

if(BUILD_WPS)
  add_subdirectory( wps )
endif(BUILD_WPS)

if(BUILD_TEMPUS_PLUGINS)
  add_subdirectory( plugins )
endif(BUILD_TEMPUS_PLUGINS)

if(BUILD_QGIS_PLUGIN)
  add_subdirectory( hmi_qgis )
endif(BUILD_QGIS_PLUGIN)

if(BUILD_INTRALYS_PLUGIN)
  add_subdirectory( intralys )
endif(BUILD_INTRALYS_PLUGIN)
