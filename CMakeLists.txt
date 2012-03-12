project( tempus )

set( CMAKE_BUILD_TYPE RELWITHDEBINFO )

add_executable( test main.cc common.cc plugin.cc pgsql_importer.cc )
target_link_libraries( test boost_graph dl pqxx )

add_library( dummy_plugin SHARED dummy_plugin.cc )

#add_library( dummy_plugin2 SHARED dummy_plugin2.cc )
