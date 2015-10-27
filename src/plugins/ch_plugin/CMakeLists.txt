add_executable( ch_contract exe_contraction.cc )
target_link_libraries( ch_contract tempus )

add_library( ch_plugin MODULE ch_plugin.cc )
target_link_libraries( ch_plugin tempus )

add_executable( test_ch test_ch.cc )
target_link_libraries( test_ch tempus )