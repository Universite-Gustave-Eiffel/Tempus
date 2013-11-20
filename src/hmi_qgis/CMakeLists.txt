include( PyQt4Macros )

if (WIN32)
	set( HOME $ENV{USERPROFILE} )
else (WIN32)
	set ( HOME $ENV{HOME} )
endif(WIN32)

SET(QGIS_DATA_DIR ${HOME}/.qgis2/)
SET(QGIS_PLUGIN_DIR "${QGIS_DATA_DIR}/python/plugins/ifsttarrouting" )

configure_file( config.py.in config.py )

FILE(GLOB INSTALLER_FILES metadata.txt *.py)
FILE(GLOB IMAGES_FILES *.png)
SET(INSTALLER_FILES ${INSTALLER_FILES}
  ${CMAKE_CURRENT_SOURCE_DIR}/../python/wps_client.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../python/tempus_request.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../python/history_file.py
  ${IMAGES_FILES}
  ${CMAKE_CURRENT_BINARY_DIR}/config.py )

FILE(GLOB UI_FILES *.ui)
PYQT4_WRAP_UI(PYUI_FILES ${UI_FILES})
PYQT4_ADD_RESOURCES(PYRC_FILES resources.qrc)
ADD_CUSTOM_TARGET(ifsttarRouting ALL DEPENDS ${PYUI_FILES} ${PYRC_FILES} )

SET(INSTALLER_FILES ${INSTALLER_FILES} ${PYUI_FILES} ${PYRC_FILES})

if (WIN32)
INSTALL(FILES ${INSTALLER_FILES} DESTINATION python/plugins/IfsttarRouting)
endif(WIN32)
