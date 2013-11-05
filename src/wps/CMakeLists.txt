find_package(LibXml2 REQUIRED) # our cmake module finds iconv on windows
find_package(FCGI REQUIRED)

include_directories( ${LIBXML2_INCLUDE_DIR} ${FCGI_INCLUDE_DIR} ../core )

add_executable( wps fastcgi.cc wps_request.cc xml_helper.cc wps_service.cc tempus_services.cc )
target_link_libraries( wps tempus ${LIBXML2_LIBRARIES} ${FCGI_LIBRARIES})

install( TARGETS wps DESTINATION bin )

if (WIN32)
    install(FILES  ${LIBXML2_LIBRARIES} ${FCGI_LIBRARIES} DESTINATION bin)
endif()
