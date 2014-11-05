-- create index on tempus.road_section(node_from);
-- create index on tempus.road_section(node_to);
-- create index on tempus.road_section using gist(geom);

do $$
DECLARE N integer;
BEGIN
    raise notice 'Splitting road sections, please wait ...';
    LOOP

-- select road sections that intersect (but not on extrema)
-- and take the first point of intersection
drop table if exists _tempus_import.crossing_sections;
create table _tempus_import.crossing_sections as
select
-- do not repeat sections
distinct on (rs1.id)
rs1.id as id1, rs2.id as id2, st_geometryn(st_multi(st_intersection(rs1.geom, rs2.geom)),1) as geom
from
tempus.road_section as rs1,
tempus.road_section as rs2
where
rs1.id <> rs2.id and
rs1.node_from <> rs2.node_to and
rs1.node_from <> rs2.node_from and
rs1.node_to <> rs2.node_from and
rs1.node_to <> rs2.node_to and
st_intersects(rs1.geom, rs2.geom) 
and
-- do not include linestring intersections
geometrytype(st_multi(st_intersection(rs1.geom, rs2.geom))) = 'MULTIPOINT'
and
-- do not cut bridges and tunnels
not rs1.tunnel and not rs2.tunnel and not rs1.bridge and not rs2.bridge
;

--
-- There are two types of intersections :
-- T-shaped : rs1 and rs2 cross on one of the ending point of either rs1 or rs2
-- X-shaped : rs1 and rs2 cross on a new unknown point

-- add new points for X-shaped intersections
insert into tempus.road_node
select
nextval('tempus.seq_road_node_id')::bigint as id,
false as bifurcation,
st_force3DZ(s.geom) as geom
from
_tempus_import.crossing_sections as s,
tempus.road_section as rs1,
tempus.road_section as rs2
where
 rs1.id = id1 and
 rs2.id = id2 and
 not st_intersects(s.geom, st_startpoint(rs1.geom)) and
 not st_intersects(s.geom, st_endpoint(rs1.geom)) and
 not st_intersects(s.geom, st_startpoint(rs2.geom)) and
 not st_intersects(s.geom, st_endpoint(rs2.geom));



-- select sections that must be split, with the split point
drop table if exists _tempus_import.splitted_sections;
create table _tempus_import.splitted_sections as
with sections_to_split as
(
select distinct on(section)
   case when st_intersects(n.geom, st_startpoint(rs1.geom)) or st_intersects(n.geom, st_endpoint(rs1.geom)) then rs2.id
        else rs1.id end as section,
       n.id as point
from
_tempus_import.crossing_sections as s,
tempus.road_section as rs1,
tempus.road_section as rs2,
tempus.road_node as n
where 
rs1.id = id1 and
rs2.id = id2 and
st_intersects( n.geom, s.geom )
)
select
  section as section_id, point as node_id,
  -- use st_snap to include intersections a little bit outside of the linestring
  st_geometryn(st_split(st_snap(rs.geom, n.geom,0.1), n.geom),1) as leftgeom,
  st_geometryn(st_split(st_snap(rs.geom, n.geom,0.1), n.geom),2) as rightgeom
from
  tempus.road_node as n,
  tempus.road_section as rs,
  sections_to_split
where
  section = rs.id and
  point = n.id;

N := count(*) from _tempus_import.splitted_sections where rightgeom is not null;
raise notice 'Splitted sections: %', N;
if N = 0 then exit; end if;

-- create the new splitted section from the old (from) point to the new one
insert into tempus.road_section
select
   nextval('_tempus_import.seq_road_section_import')::bigint as id,
   road_type,
   node_from,
   s.node_id as node_to,
   traffic_rules_ft,
   traffic_rules_tf,
   st_length(leftgeom),
   car_speed_limit,
   road_name,
   lane,
   roundabout,
   bridge,
   tunnel,
   ramp,
   tollway,
   leftgeom
from _tempus_import.splitted_sections as s,
  tempus.road_section as rs
where
  s.section_id = rs.id
 and s.rightgeom is not null;

  -- create the new splitted section from the new point to the old (to) one
insert into tempus.road_section
select
   nextval('_tempus_import.seq_road_section_import')::bigint as id,
   road_type,
   s.node_id as node_from,
   node_to,
   traffic_rules_ft,
   traffic_rules_tf,
   st_length(rightgeom),
   car_speed_limit,
   road_name,
   lane,
   roundabout,
   bridge,
   tunnel,
   ramp,
   tollway,
   rightgeom
from _tempus_import.splitted_sections as s,
  tempus.road_section as rs
where
  s.section_id = rs.id
and
  s.rightgeom is not null;

-- delete origin sections
delete from tempus.road_section
where id in (select section_id from _tempus_import.splitted_sections where rightgeom is not null);

end loop;
end$$;
