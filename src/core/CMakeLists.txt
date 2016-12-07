set( HEADER_FILES
  abscissa.hh
  application.hh
  base.hh
  cast.hh
  common.hh
  cost.hh
  datetime.hh
  db.hh
  io.hh
  multimodal_graph.hh
  path_trace.hh
  plugin.hh
  plugin_factory.hh
  poi.hh
  point.hh
  progression.hh
  property.hh
  public_transport_graph.hh
  request.hh
  reverse_multimodal_graph.hh
  road_graph.hh
  roadmap.hh
  sub_map.hh
  transport_modes.hh
  variant.hh
  serializers.hh
  routing_data.hh
  routing_data_builder.hh
  multimodal_graph_builder.hh
  ch_routing_data.hh
)

set( UTILS_HEADER_FILES
  utils/associative_property_map_default_value.hh
  utils/field_property_accessor.hh
  utils/function_property_accessor.hh
  utils/graph_db_link.hh
  utils/timer.hh
)

set( SOURCE_FILES
    io.cc
    progression.cc
    cost.cc
    abscissa.cc
    road_graph.cc
    public_transport_graph.cc
    multimodal_graph.cc
    reverse_multimodal_graph.cc
    poi.cc
    point.cc
    transport_modes.cc
    request.cc
    roadmap.cc
    application.cc
    plugin.cc
    plugin_factory.cc
    db.cc
    utils/graph_db_link.cc
    variant.cc
    serializers.cc
    routing_data.cc
    routing_data_builder.cc
    multimodal_graph_builder.cc
    ch_routing_data.cc
)

set (PYTHON_SOURCE_FILES python_binding.cc )

if (ENABLE_SEGMENT_ALLOCATOR)
  set( SOURCE_FILES ${SOURCE_FILES} utils/segment_allocator.cc )
  set( UTILS_HEADER_FILES ${UTILS_HEADER_FILES} utils/segment_allocator.hh )
endif ()

if (WIN32)
    add_library( tempus STATIC ${HEADER_FILES} ${UTILS_HEADER_FILES} ${SOURCE_FILES} )
else ()
    add_library( tempus SHARED ${SOURCE_FILES} )
endif ()


target_link_libraries( tempus ${PostgreSQL_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

add_library( pytempus SHARED ${PYTHON_SOURCE_FILES} )
set_target_properties( pytempus PROPERTIES PREFIX "" )
set_target_properties( pytempus PROPERTIES OUTPUT_NAME "tempus")
target_link_libraries( pytempus ${PostgreSQL_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} tempus)


install(TARGETS tempus DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include/tempus)
install(FILES ${UTILS_HEADER_FILES} DESTINATION include/tempus/utils)

