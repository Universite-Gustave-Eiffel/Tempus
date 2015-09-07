include_directories( ../core )

add_executable(graph_mem_usage graph_mem_usage.cc)
target_link_libraries( graph_mem_usage tempus )

install(TARGETS graph_mem_usage DESTINATION bin)

