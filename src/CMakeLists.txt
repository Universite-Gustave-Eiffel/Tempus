
set( BUILD_CORE ON CACHE BOOL "Build Tempus core ?" )
set( BUILD_WPS ON CACHE BOOL "Build WPS server ?" )
set( BUILD_QGIS_PLUGIN ON CACHE BOOL "Build QGIS plugin ?" )
set( BUILD_TEMPUS_PLUGINS ON CACHE BOOL "Build Tempus plugins ?" )
set( BUILD_OSM2SHP ON CACHE BOOL "Build osm2shp ?" )

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

if(BUILD_OSM2SHP)
  add_subdirectory( loader )
endif()

if(WIN32)
    install(DIRECTORY python DESTINATION ${TEMPUS_INSTALL_DATA_DIRECTORY})
    install(FILES batch/random_batch.py DESTINATION ${TEMPUS_INSTALL_DATA_DIRECTORY}/batch)
endif()

