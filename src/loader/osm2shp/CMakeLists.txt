find_package(LibXml2 REQUIRED) # our cmake module finds iconv on windows
find_package(GLIB REQUIRED)
find_package(SHP REQUIRED)

include_directories(${SHP_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} ${GLIB_INCLUDE_DIR})

add_executable(osm2shp osm2shp.c)
target_link_libraries(osm2shp ${LIBXML2_LIBRARIES} ${SHP_LIBRARIES} ${GLIB_LIBRARIES})
