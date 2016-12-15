find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/osm/*.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

include_directories(${PROTOBUF_INCLUDE_DIR} ../../core ${CMAKE_CURRENT_BINARY_DIR})

add_executable(osm2tempus main.cpp pgsql_writer.cpp single_pass_reader.cpp two_pass_reader.cpp data_profile.cpp tempus_profile.cpp geom.cpp ${ProtoSources} ${ProtoHeaders})

target_link_libraries(osm2tempus ${ZLIB_LIBRARIES} tempus ws2_32.lib ${PROTOBUF_LIBRARY})
if (MSVC)
  target_link_libraries(osm2tempus ws2_32.lib)
endif (MSVC)


install(TARGETS osm2tempus DESTINATION bin)

