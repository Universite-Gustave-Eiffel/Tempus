find_package(LibXml2 REQUIRED) # our cmake module finds iconv on windows
find_package(FCGI REQUIRED)

include_directories( ${LIBXML2_INCLUDE_DIR} ${FCGI_INCLUDE_DIR} ../core )

add_executable( tempus_wps fastcgi.cc wps_request.cc xml_helper.cc wps_service.cc tempus_services.cc )
target_link_libraries( tempus_wps tempus ${LIBXML2_LIBRARIES} ${FCGI_LIBRARIES} ${PYTHON_LIBRARIES} )

install( TARGETS tempus_wps DESTINATION bin )

if (WIN32)
    message( "will install " ${LIBXML2_DLL} ${FGCI_DLL})
    install(FILES ${LIBXML2_DLL} ${FGCI_DLL} DESTINATION bin)
endif()
