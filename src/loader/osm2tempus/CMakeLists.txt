find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(../../core)

add_executable(osm2tempus main.cpp)
target_link_libraries(osm2tempus ${PROTOBUF_LIBRARIES} ${ZLIB_LIBRARIES} osmpbf tempus)

install(TARGETS osm2tempus DESTINATION bin)

