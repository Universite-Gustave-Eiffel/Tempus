
if(WIN32)
else()
    add_definitions(-DLARGEFILE_SOURCE )
    set(SHP_LIBRARIES shp)
    set(LIBXML2_LIBRARIES xml2)
endif()

link_directories(${LIBXML2_LIBRARY_DIRS})
include_directories(${LIBXML2_INCLUDE_DIR})

add_executable(osm2shp osm2shp.c)
target_link_libraries(osm2shp ${LIBXML2_LIBRARIES} ${ICONV_LIBS} ${SHP_LIBRARIES})
