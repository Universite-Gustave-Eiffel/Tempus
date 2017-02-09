do $$
begin
raise notice '==== Creating road geometry indexes ===';
end$$;

create index on tempus.road_node using gist(geom);
create index on tempus.road_section using gist(geom);
create index on tempus.road_section(node_from);
create index on tempus.road_section(node_to);
