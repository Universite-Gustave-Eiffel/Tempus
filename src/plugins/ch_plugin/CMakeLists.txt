add_library( ch_plugin MODULE ch_plugin.cc )
target_link_libraries( ch_plugin tempus )

add_executable( ch_preprocess ch_preprocess.cc ch_preprocess_main.cc )
target_link_libraries( ch_preprocess tempus )
