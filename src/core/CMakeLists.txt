set( HEADER_FILES
    common.hh
    cast.hh
    cost.hh
    io.hh
    property.hh
    base.hh
    datetime.hh
    progression.hh
    abscissa.hh
    road_graph.hh
    public_transport_graph.hh
    poi.hh
    point.hh
    transport_modes.hh
    multimodal_graph.hh 
    reverse_multimodal_graph.hh
    request.hh
    roadmap.hh
    application.hh plugin.hh 
    pgsql_importer.hh 
    plugin.hh 
    db.hh 
    utils/graph_db_link.hh 
    )

set( SOURCE_FILES 
    io.cc
    progression.cc
    cost.cc
    abscissa.cc
    road_graph.cc
    multimodal_graph.cc 
    reverse_multimodal_graph.cc
    poi.cc
    point.cc
    transport_modes.cc
    request.cc
    roadmap.cc
    application.cc plugin.cc 
    pgsql_importer.cc 
    plugin.cc 
    db.cc 
    utils/graph_db_link.cc 
    )

if (WIN32)
    add_library( tempus STATIC ${HEADER_FILES} ${SOURCE_FILES} )
else ()
    add_library( tempus SHARED ${SOURCE_FILES} )
endif ()

target_link_libraries( tempus ${PostgreSQL_LIBRARIES} ${Boost_LIBRARIES} )

add_executable( sample_app sample_app.cc )
target_link_libraries( sample_app tempus )

install(TARGETS tempus DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)

