include_directories(../core)

add_library( sample_multi_plugin MODULE sample_multi_plugin/sample_multi_plugin.cc )
target_link_libraries( sample_multi_plugin tempus )

add_library( sample_road_plugin MODULE sample_road_plugin/sample_road_plugin.cc )
target_link_libraries( sample_road_plugin tempus )

add_library( sample_pt_plugin MODULE sample_pt_plugin/sample_pt_plugin.cc )
target_link_libraries( sample_pt_plugin tempus )

add_library( forbidden_movements_plugin MODULE forbidden_movements_plugin/forbidden_movements_plugin.cc )
target_link_libraries( forbidden_movements_plugin tempus )

install( TARGETS 
    sample_multi_plugin 
    sample_road_plugin 
    sample_pt_plugin 
    forbidden_movements_plugin
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    )
