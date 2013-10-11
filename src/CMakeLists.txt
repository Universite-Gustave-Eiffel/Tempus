
set( BUILD_CORE ON CACHE BOOL "Build Tempus core ?" )
set( BUILD_TESTS OFF CACHE BOOL "Build unit tests ?" )
set( BUILD_WPS OFF CACHE BOOL "Build WPS server ?" )
set( BUILD_QGIS_PLUGIN OFF CACHE BOOL "Build QGIS plugin ?" )
set( BUILD_INTRALYS_PLUGIN OFF CACHE BOOL "Build Intralys plugin ?" )
set( BUILD_TEMPUS_PLUGINS ON CACHE BOOL "Build Tempus plugins ?" )

if(BUILD_CORE)
  add_subdirectory( core )
endif(BUILD_CORE)

if(BUILD_TESTS)
  add_subdirectory( tests )
endif(BUILD_TESTS)

if(BUILD_WPS)
  add_subdirectory( wps )
endif(BUILD_WPS)

if(BUILD_TEMPUS_PLUGINS)
  add_subdirectory( plugins )
endif(BUILD_TEMPUS_PLUGINS)

if(BUILD_QGIS_PLUGIN)
  add_subdirectory( hmi_qgis )
endif(BUILD_QGIS_PLUGIN)

if(BUILD_INTRALYS_PLUGIN)
  add_subdirectory( intralys )
endif(BUILD_INTRALYS_PLUGIN)
