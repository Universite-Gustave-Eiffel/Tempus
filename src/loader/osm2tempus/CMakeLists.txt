find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS osmpbf/osmformat.proto osmpbf/fileformat.proto)

include_directories(${PROTOBUF_INCLUDE_DIR} ../../core ${CMAKE_CURRENT_BINARY_DIR})

add_executable(osm2tempus
  ${PROTO_SRCS}
  
  main.cpp
  pgsql_writer.cpp
  single_pass_reader.cpp
  two_pass_reader.cpp
  data_profile.cpp
  tempus_profile.cpp
  geom.cpp)
target_link_libraries(osm2tempus ${PROTOBUF_LIBRARIES} ${ZLIB_LIBRARIES} tempus)
if (MSVC)
  # we use protobuf as a dll
  add_definitions("/DPROTOBUF_USE_DLLS")
  target_link_libraries(osm2tempus ws2_32.lib)
endif (MSVC)

install(TARGETS osm2tempus DESTINATION bin)

