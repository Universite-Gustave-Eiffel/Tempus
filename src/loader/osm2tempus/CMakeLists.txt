find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(../../core)

add_executable(osm2tempus
  osmpbf/osmformat.pb.cc
  osmpbf/fileformat.pb.cc
  
  main.cpp
  pgsql_writer.cpp
  single_pass_reader.cpp
  two_pass_reader.cpp
  data_profile.cpp
  tempus_profile.cpp
  geom.cpp)
target_link_libraries(osm2tempus ${PROTOBUF_LIBRARIES} ${ZLIB_LIBRARIES} tempus)

install(TARGETS osm2tempus DESTINATION bin)

