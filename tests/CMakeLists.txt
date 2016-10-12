include_directories( ../src/core )

add_executable( test_core tests.cc routing_data_builder_tests.cc main.cc )
target_link_libraries( test_core tempus )

add_executable( test_pt timetable_tests.cc main.cc )
target_link_libraries( test_pt tempus )

add_test( test_core ${EXECUTABLE_OUTPUT_PATH}/test_core )


