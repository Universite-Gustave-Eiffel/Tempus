include_directories(../core)

add_library( sample_multi_plugin MODULE sample_multi_plugin/sample_multi_plugin.cc )
target_link_libraries( sample_multi_plugin tempus )

add_library( sample_road_plugin MODULE sample_road_plugin/sample_road_plugin.cc )
target_link_libraries( sample_road_plugin tempus )

add_library( sample_pt_plugin MODULE sample_pt_plugin/sample_pt_plugin.cc )
target_link_libraries( sample_pt_plugin tempus )

add_library( homogen_tmr_road_plugin MODULE homogen_tmr_road_plugin/homogen_tmr_road_plugin.cc )
target_link_libraries( homogen_tmr_road_plugin tempus )

add_library( heterogen_tmr_road_plugin MODULE heterogen_tmr_road_plugin/heterogen_tmr_road_plugin.cc )
target_link_libraries( heterogen_tmr_road_plugin tempus )

add_library( dynamic_multi_plugin MODULE dynamic_multi_plugin/dynamic_multi_plugin.cc )
target_link_libraries( dynamic_multi_plugin tempus )

if (WIN32)
install( TARGETS 
    sample_multi_plugin 
    sample_road_plugin 
    sample_pt_plugin 
    homogen_tmr_road_plugin
    heterogen_tmr_road_plugin
    dynamic_multi_plugin
    LIBRARY DESTINATION bin
    )
else()
install( TARGETS 
    sample_multi_plugin 
    sample_road_plugin 
    sample_pt_plugin 
    homogen_tmr_road_plugin
    heterogen_tmr_road_plugin
    dynamic_multi_plugin
    LIBRARY DESTINATION lib
    )
endif()
