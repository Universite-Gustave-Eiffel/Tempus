find_package(LibXml2 REQUIRED) # our cmake module finds iconv on windows
find_package(GLIB REQUIRED)
find_package(SHP REQUIRED)

include_directories(${SHP_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} ${GLIB_INCLUDE_DIR})

add_executable(osm2shp osm2shp.c)
target_link_libraries(osm2shp ${LIBXML2_LIBRARIES} ${SHP_LIBRARIES} ${GLIB_LIBRARIES})

add_executable(osm2shp++ osm2shp.cc)
target_link_libraries(osm2shp++ ${LIBXML2_LIBRARIES} ${SHP_LIBRARIES} )

file( DOWNLOAD "http://api.openstreetmap.org/api/0.6/map?bbox=11.5418,48.14,11.543,48.143" ${CMAKE_CURRENT_BINARY_DIR}/test_osm2shp.osm INACTIVITY_TIMEOUT .5 SHOW_PROGRESS )

add_test(test_osm2shp ${EXECUTABLE_OUTPUT_PATH}/osm2shp ${CMAKE_CURRENT_BINARY_DIR}/test_osm2shp.osm)
add_test(test_osm2shp++ ${EXECUTABLE_OUTPUT_PATH}/osm2shp++ ${CMAKE_CURRENT_BINARY_DIR}/test_osm2shp.osm)
