project( tempus )
cmake_minimum_required(VERSION 2.6)

set( CMAKE_BUILD_TYPE RELWITHDEBINFO )

find_package(Boost)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
	link_directories(${Boost_LIBRARY_DIRS})
    add_definitions( "-DHAS_BOOST" )
endif (Boost_FOUND)

# Core source files
set( SOURCE_FILES common.cc plugin.cc pgsql_importer.cc plugin.cc db.cc )

if (WIN32)
	set( PQ_INCLUDE_DIR "C:/Program Files/PostgreSQL/9.1/include" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "C:/Program Files/PostgreSQL/9.1/lib" CACHE PATH "libpq lib directory" )
	set( PQ_LIBS libpq )
	# With Visual C++, DLLs must be compiled with the core, in order to resolve external symbols
	set( DLL_SOURCES ${SOURCE_FILES} )
	# Disable verbose warnings
	add_definitions( "/D_CRT_SECURE_NO_WARNINGS /wd4290" )
else (WIN32)
	set( PQ_INCLUDE_DIR "/usr/include/postgresql" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "/usr/lib" CACHE PATH "libpq lib directory" )
	set( PQ_LIBS pq dl boost_program_options )
	# With G++/Linux, .so must not be compiled with sources of the core. Otherwise global symbols get allocated
	# many times, and we get a double-free corruption on exit.
	set( DLL_SOURCES "" )
endif (WIN32)

include_directories( ${PQ_INCLUDE_DIR} )
link_directories( ${PQ_LIBS_DIR} )

add_executable( test main.cc ${SOURCE_FILES} )
target_link_libraries( test ${PQ_LIBS} )

add_library( tempus STATIC ${SOURCE_FILES} )
target_link_libraries( tempus ${PQ_LIBS} )

add_library( dummy_plugin MODULE dummy_plugin.cc ${DLL_SOURCES} )
target_link_libraries( dummy_plugin ${PQ_LIBS} )

add_library( sample_road_plugin MODULE sample_road_plugin.cc ${DLL_SOURCES} )
target_link_libraries( sample_road_plugin ${PQ_LIBS} )

add_subdirectory( tests )
