find_package(LibXml2 REQUIRED) # our cmake module finds iconv on windows
find_package(SHP REQUIRED)

include_directories(${SHP_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} )

find_path( OSM_TEST_FILE test_osm2shp.osm ${CMAKE_CURRENT_BINARY_DIR} )
if (NOT OSM_TEST_FILE)
    set(OSM_TEST_FILE ${CMAKE_CURRENT_BINARY_DIR}/test_osm2shp.osm)
    file( DOWNLOAD "http://api.openstreetmap.org/api/0.6/map?bbox=11.5418,48.14,11.543,48.143" ${OSM_TEST_FILE} INACTIVITY_TIMEOUT .5 SHOW_PROGRESS )
else()
    set(OSM_TEST_FILE ${OSM_TEST_FILE}/test_osm2shp.osm)
endif()

add_executable(osm2shp++ osm2shp.cc)
target_link_libraries(osm2shp++ ${LIBXML2_LIBRARIES} ${SHP_LIBRARIES} )

add_test(test_osm2shp++ ${EXECUTABLE_OUTPUT_PATH}/osm2shp++ ${OSM_TEST_FILE})

install(TARGETS osm2shp++ DESTINATION bin)

if(WIN32)
    install(FILES ${SHP_DLL} DESTINATION bin)
endif()
