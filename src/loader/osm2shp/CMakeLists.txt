
find_package(LibXml2 REQUIRED) # our cmake module finds iconv on windows

if(WIN32)
else()
    #add_definitions(-DLARGEFILE_SOURCE )
    set(SHP_LIBRARIES shp)
    set(GLIB_INCLUDE_DIR /usr/include/glib-2.0 /usr/lib/x86_64-linux-gnu/glib-2.0/include)
    set(GLIB_LIBRARIES glib-2.0)
endif()

link_directories(${LIBXML2_LIBRARY_DIRS})
include_directories(${LIBXML2_INCLUDE_DIR} ${GLIB_INCLUDE_DIR})

add_executable(osm2shp osm2shp.c)
target_link_libraries(osm2shp ${LIBXML2_LIBRARIES} ${SHP_LIBRARIES} ${GLIB_LIBRARIES})
