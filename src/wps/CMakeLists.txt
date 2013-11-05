find_package(LibXml2 REQUIRED) # our cmake module finds iconv on windows
find_package(FCGI REQUIRED)

link_directories(  ${ICONV_LIBRARY_DIRS} ${FCGI_LIBRARY_DIRS})
include_directories( ${LIBXML2_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${FCGI_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../core )

add_executable( wps fastcgi.cc wps_request.cc xml_helper.cc wps_service.cc tempus_services.cc )
target_link_libraries( wps tempus ${LIBXML2_LIBRARIES} ${FCGI_LIBRARIES} ${Boost_LIBRARIES} )

install( TARGETS wps DESTINATION bin )

if (WIN32)
    install(FILES  ${LIBXML2_LIBRARIES} ${FCGI_LIBRARIES} DESTINATION bin)
endif()
