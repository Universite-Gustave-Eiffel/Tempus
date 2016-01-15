include_directories( ../core )

add_executable(graph_dumper graph_dumper.cc)
target_link_libraries( graph_dumper tempus )

install(TARGETS graph_dumper DESTINATION bin)

