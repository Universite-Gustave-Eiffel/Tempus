find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)

#include_directories(${PROTOBUF_INCLUDE_DIR})

add_executable(osm2tempus main.cpp)
target_link_libraries(osm2tempus ${PROTOBUF_LIBRARIES} ${ZLIB_LIBRARIES} osmpbf)

install(TARGETS osm2tempus DESTINATION bin)

