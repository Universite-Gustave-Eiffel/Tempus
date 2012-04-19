set( CPPUNIT_INCLUDE_DIR "" CACHE PATH "CppUnit includes directory")
set( CPPUNIT_LIBRARY_DIRS "" CACHE PATH "CppUnit libraries directory")
if (WIN32)
  set( CPPUNIT_LIBS libcppunit )
else (WIN32)
  set( CPPUNIT_LIBS cppunit )
endif (WIN32)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../core ${PQ_INCLUDE_DIR} )
link_directories( ${CMAKE_CURRENT_BUILD_DIR} )

add_executable( test_core tests.cc main.cc )
target_link_libraries( test_core ${CPPUNIT_LIBS} ${PQ_LIBS} tempus )