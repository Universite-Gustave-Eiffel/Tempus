
set( BUILD_CORE ON CACHE BOOL "Build Tempus core ?" )
set( BUILD_WPS ON CACHE BOOL "Build WPS server ?" )
set( BUILD_QGIS_PLUGIN ON CACHE BOOL "Build QGIS plugin ?" )
set( BUILD_TEMPUS_PLUGINS ON CACHE BOOL "Build Tempus plugins ?" )
set( BUILD_OSM2SHP ON CACHE BOOL "Build osm2shp ?" )
set( BUILD_TOOLS ON CACHE BOOL "Build tools ?" )

if (NOT WIN32)
  set( ENABLE_SEGMENT_ALLOCATOR OFF CACHE BOOL "Allow to use a segment allocator ?" )
endif()

if ( ENABLE_SEGMENT_ALLOCATOR )
  set ( ENABLE_SEGMENT_ALLOCATOR_B 1 )
else()
  set ( ENABLE_SEGMENT_ALLOCATOR_B 0 )
endif()

configure_file( config.hh.in config.hh )

include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

if(BUILD_CORE)
  add_subdirectory( core )
endif()

if(BUILD_WPS)
  add_subdirectory( wps )
endif()

if(BUILD_TEMPUS_PLUGINS)
  add_subdirectory( plugins )
endif()

if(BUILD_QGIS_PLUGIN)
  add_subdirectory( hmi_qgis )
endif()

add_subdirectory( loader )

if(BUILD_TOOLS)
  add_subdirectory( tools )
endif()

add_subdirectory( standalone )

if(WIN32)
    install(DIRECTORY python DESTINATION ${TEMPUS_INSTALL_DATA_DIRECTORY})
    install(FILES batch/random_batch.py DESTINATION ${TEMPUS_INSTALL_DATA_DIRECTORY}/batch)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config.hh DESTINATION include/tempus)

