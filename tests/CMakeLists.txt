include_directories( ../src/core )

add_executable( test_core tests.cc main.cc )
target_link_libraries( test_core tempus )

add_test( test_core ${EXECUTABLE_OUTPUT_PATH}/test_core )

