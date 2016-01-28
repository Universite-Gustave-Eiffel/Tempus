include_directories( ../core )

add_executable( tempus_standalone main.cc )
target_link_libraries( tempus_standalone tempus )

install(TARGETS tempus_standalone DESTINATION bin)
