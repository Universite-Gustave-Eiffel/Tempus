# config file generation
# FIXME use installation directory on install
set(TEMPUS_DATA_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../data")
configure_file( tempus_config.hh.in tempus_config.hh )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

# Core source files
set( SOURCE_FILES 
	common.hh 
	multimodal_graph.hh 
	application.hh plugin.hh 
	pgsql_importer.hh 
	plugin.hh 
	db.hh 
	utils/graph_db_link.hh 
	common.cc 
	multimodal_graph.cc 
	application.cc plugin.cc 
	pgsql_importer.cc 
	plugin.cc 
	db.cc 
	utils/graph_db_link.cc 
	)

if (WIN32)
	# With Visual C++, DLLs must be compiled with the core, in order to resolve external symbols
	set( DLL_SOURCES ${SOURCE_FILES} )
else (WIN32)
	# With G++/Linux, .so must not be compiled with sources of the core. Otherwise global symbols get allocated
	# many times, and we get a double-free corruption on exit.
	set( DLL_SOURCES "" )
endif (WIN32)

# TO BE FIXED
if (WIN32)
add_library( tempus STATIC ${SOURCE_FILES} )
else (WIN32)
add_library( tempus SHARED ${SOURCE_FILES} )
endif (WIN32)
target_link_libraries( tempus ${PQ_LIBS} )

add_executable( sample_app sample_app.cc )
target_link_libraries( sample_app tempus )
install(TARGETS tempus DESTINATION lib)
