find_package(LibXml2 REQUIRED)

set( ICONV_INCLUDE_DIR "" CACHE PATH "libiconv includes directory")
set( ICONV_LIBRARY_DIRS "" CACHE PATH "libiconv libraries directory")
if (WIN32)
  set( ICONV_LIBS iconv )
else (WIN32)
  set (ICONV_LIBS )
endif (WIN32)

set( FCGI_INCLUDE_DIR "" CACHE PATH "fastcgi includes directory")
set( FCGI_LIBRARY_DIRS "" CACHE PATH "fastcgi libraries directory")
if (WIN32)
  set( FCGI_LIBS libfcgi )
else (WIN32)
  set( FCGI_LIBS fcgi fcgi++ )
endif (WIN32)

link_directories(  ${ICONV_LIBRARY_DIRS} ${FCGI_LIBRARY_DIRS} ${CMAKE_CURRENt_BUILD_DIR} )
include_directories( ${LIBXML2_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${FCGI_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../core )

add_executable( wps fastcgi.cc wps_request.cc xml_helper.cc wps_service.cc tempus_services.cc )
target_link_libraries( wps ${LIBXML2_LIBRARIES} ${ICONV_LIBS} ${FCGI_LIBS} ${LIBTEMPUS} boost_thread )
