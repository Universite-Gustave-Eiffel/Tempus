project( tempus )
cmake_minimum_required(VERSION 2.6)

set( CMAKE_BUILD_TYPE RELWITHDEBINFO )

find_package(Boost)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
    add_definitions( "-DHAS_BOOST" )
endif (Boost_FOUND)

# Core source files
set( SOURCE_FILES common.cc plugin.cc pgsql_importer.cc plugin.cc )

if (WIN32)
	set( PQ_INCLUDE_DIR "C:/Program Files/PostgreSQL/9.1/include" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "C:/Program Files/PostgreSQL/9.1/lib" CACHE PATH "libpq lib directory" )
	set( PQXX_INCLUDE_DIR "C:/Program Files/libpqxx-4.0/include" CACHE PATH "libpqxx include directory" )
	set( PQXX_LIBS_DIR "C:/Program Files/libpqxx-4.0/lib" CACHE PATH "libpqxx lib directory" )
	set( PQ_LIBS libpq libpqxx )
	# With Visual C++, DLLs must be compiled with the core, in order to resolve external symbols
	set( DLL_SOURCES ${SOURCE_FILES} )
else (WIN32)
	set( PQ_INCLUDE_DIR "/usr/include/postgresql" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "/usr/lib" CACHE PATH "libpq lib directory" )
	set( PQXX_INCLUDE_DIR "/usr/include/pqxx" CACHE PATH "libpqxx include directory" )
	set( PQXX_LIBS_DIR "/usr/lib" CACHE PATH "libpqxx lib directory" )
	set( PQ_LIBS pq pqxx dl boost_program_options )
	set( SOURCE_FILES common.cc plugin.cc pgsql_importer.cc plugin.cc )
	# With G++/Linux, .so must not be compiled with sources of the core. Otherwise global symbols get allocated
	# many times, and we get a double-free corruption on exit.
	set( DLL_SOURCES "" )
endif (WIN32)

include_directories(${PQ_INCLUDE_DIR} ${PQXX_INCLUDE_DIR})
link_directories( ${PQ_LIBS_DIR} ${PQXX_LIBS_DIR} )

add_executable( test main.cc ${SOURCE_FILES} )
target_link_libraries( test ${PQ_LIBS} )

add_library( dummy_plugin MODULE dummy_plugin.cc ${DLL_SOURCES} )
target_link_libraries( dummy_plugin ${PQ_LIBS} )

add_library( sample_road_plugin MODULE sample_road_plugin.cc ${DLL_SOURCES} )
target_link_libraries( sample_road_plugin ${PQ_LIBS} )
