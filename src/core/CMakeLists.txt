# Core source files
set( SOURCE_FILES common.cc multimodal_graph.cc application.cc plugin.cc pgsql_importer.cc plugin.cc db.cc )

if (WIN32)
	# With Visual C++, DLLs must be compiled with the core, in order to resolve external symbols
	set( DLL_SOURCES ${SOURCE_FILES} )
else (WIN32)
	# With G++/Linux, .so must not be compiled with sources of the core. Otherwise global symbols get allocated
	# many times, and we get a double-free corruption on exit.
	set( DLL_SOURCES "" )
endif (WIN32)

add_executable( sample_app sample_app.cc ${SOURCE_FILES} )
target_link_libraries( sample_app ${PQ_LIBS} )

# TO BE FIXED
if (WIN32)
add_library( tempus STATIC ${SOURCE_FILES} )
else (WIN32)
add_library( tempus SHARED ${SOURCE_FILES} )
endif (WIN32)
target_link_libraries( tempus ${PQ_LIBS} )
