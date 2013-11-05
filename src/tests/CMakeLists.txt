include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../core ${PQ_INCLUDE_DIR} )

add_executable( test_core tests.cc main.cc )
target_link_libraries( test_core  tempus ${Boost_LIBRARIES} )

add_test( test_core ${EXECUTABLE_OUTPUT_PATH}/test_core )
#add_test( test_wps  ${CMAKE_CURRENT_SOURCE_DIR}/test_wps.py )
