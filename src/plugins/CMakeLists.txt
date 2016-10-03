include_directories(../core)

if (WIN32)
  add_library( mm_lib STATIC mm_lib/speed_profile.hh mm_lib/speed_profile.cc )
else()
  add_library( mm_lib SHARED mm_lib/speed_profile.cc )
endif()

add_library( sample_multi_plugin MODULE sample_multi_plugin/sample_multi_plugin.cc )
target_link_libraries( sample_multi_plugin tempus )

add_library( sample_road_plugin MODULE sample_road_plugin/sample_road_plugin.cc )
target_link_libraries( sample_road_plugin tempus )

add_library( sample_pt_plugin MODULE sample_pt_plugin/sample_pt_plugin.cc )
target_link_libraries( sample_pt_plugin tempus )

include_directories(.)

add_library( dynamic_multi_plugin MODULE dynamic_multi_plugin/dynamic_multi_plugin.cc )
target_link_libraries( dynamic_multi_plugin tempus mm_lib )

add_subdirectory( ch_plugin )

add_subdirectory( astar_road_plugin )

if (WIN32)
install( TARGETS 
    sample_multi_plugin 
    sample_road_plugin 
    sample_pt_plugin 
    dynamic_multi_plugin
    LIBRARY DESTINATION bin
    )
else()
install( TARGETS 
    sample_multi_plugin 
    sample_road_plugin 
    sample_pt_plugin 
    dynamic_multi_plugin
    mm_lib
    LIBRARY DESTINATION lib/tempus
    )
endif()
