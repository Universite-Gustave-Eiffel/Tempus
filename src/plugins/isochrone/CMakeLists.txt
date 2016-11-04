add_library( isochrone_plugin MODULE isochrone_plugin.cpp )
target_link_libraries( isochrone_plugin tempus mm_lib )

if (WIN32)
install( TARGETS isochrone_plugin
         LIBRARY DESTINATION bin )
else()
  install( TARGETS isochrone_plugin
    LIBRARY DESTINATION lib/tempus )
endif()
