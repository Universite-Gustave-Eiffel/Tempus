
add_executable( tests tests.cc main.cc )
link_directories( .. )
target_link_libraries( tests cppunit ${PQ_LIBS} tempus )