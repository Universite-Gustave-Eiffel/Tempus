
add_executable( tests tests.cc main.cc )
link_directories( ${CMAKE_CURRENT_BUILD_DIR} )
target_link_libraries( tests ${CPPUNIT_LIBS} ${PQ_LIBS} tempus )