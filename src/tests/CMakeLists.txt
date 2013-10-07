set( CPPUNIT_INCLUDE_DIR "" CACHE PATH "CppUnit includes directory")
set( CPPUNIT_LIBRARY_DIRS "" CACHE PATH "CppUnit libraries directory")
set( CPPUNIT_LIBS cppunit )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../core ${PQ_INCLUDE_DIR} ${CPPUNIT_INCLUDE_DIR} )
link_directories( ${CMAKE_CURRENT_BUILD_DIR}/core ${CPPUNIT_LIBRARY_DIRS} )

add_executable( test_core tests.cc main.cc )
target_link_libraries( test_core ${CPPUNIT_LIBS} ${PQ_LIBS} ${LIBTEMPUS} )

add_test( test_core ${EXECUTABLE_OUTPUT_PATH}/test_core )
