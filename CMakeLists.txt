project( tempus )
cmake_minimum_required(VERSION 2.6)

set( CMAKE_BUILD_TYPE RELWITHDEBINFO )

find_package(Boost)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
    add_definitions( "-DHAS_BOOST" )
endif (Boost_FOUND)

if (WIN32)
	set( PQ_INCLUDE_DIR "C:/Program Files/PostgreSQL/9.1/include" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "C:/Program Files/PostgreSQL/9.1/lib" CACHE PATH "libpq lib directory" )
	set( PQXX_INCLUDE_DIR "C:/Program Files/libpqxx-4.0/include" CACHE PATH "libpqxx include directory" )
	set( PQXX_LIBS_DIR "C:/Program Files/libpqxx-4.0/lib" CACHE PATH "libpqxx lib directory" )
	set( PQ_LIBS libpq libpxx libtempus_core )
else (WIN32)
	set( PQ_INCLUDE_DIR "/usr/include/postgresql" CACHE PATH "libpq include directory" )
	set( PQ_LIBS_DIR "/usr/lib" CACHE PATH "libpq lib directory" )
	set( PQXX_INCLUDE_DIR "/usr/include/pqxx" CACHE PATH "libpqxx include directory" )
	set( PQXX_LIBS_DIR "/usr/lib" CACHE PATH "libpqxx lib directory" )
	set( PQ_LIBS pq pqxx dl tempus_core )
endif (WIN32)

include_directories(${PQ_INCLUDE_DIR} ${PQXX_INCLUDE_DIR})
link_directories( ${PQ_LIBS_DIR} ${PQXX_LIBS_DIR} )

# the core library
add_library( tempus_core SHARED common.cc plugin.cc pgsql_importer.cc plugin.cc )
target_link_libraries( tempus_core ${PQ_LIBS} )

add_executable( test main.cc )
target_link_libraries( test ${PQ_LIBS} )

add_library( dummy_plugin MODULE dummy_plugin.cc )
target_link_libraries( dummy_plugin ${PQ_LIBS} )
