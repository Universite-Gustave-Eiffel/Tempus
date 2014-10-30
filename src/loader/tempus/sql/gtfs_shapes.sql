--
-- shapes.txt import

drop table if exists _tempus_import.section_shape;
create table _tempus_import.section_shape as
select distinct
  -- first_value: we consider here the first trip for that pair (stop_from, stop_to)
  stop_from, stop_to, first_value(shape_id) over (partition by stop_from, stop_to order by stop_from, stop_to) as shape_id
from
  tempus.pt_section,
  tempus.pt_stop_time as st1,
  tempus.pt_stop_time as st2,
  _tempus_import.pt_trip_idmap,
  _tempus_import.trips
where
  st1.stop_id = stop_from and
  st2.stop_id = stop_to and
  st1.stop_sequence +1 = st2.stop_sequence and
  st1.trip_id = st2.trip_id and
  pt_trip_idmap.id = st1.trip_id and
  trips.trip_id = pt_trip_idmap.vendor_id
  ;
create index on _tempus_import.section_shape(stop_from);
create index on _tempus_import.section_shape(stop_to);

drop table if exists _tempus_import.shape_points;
create table _tempus_import.shape_points as
select 
  shape_id,
  shape_pt_sequence,
  st_transform(('srid=4326;point(' || shape_pt_lon || ' ' || shape_pt_lat || ')')::geometry, 2154) as geom
from
  _tempus_import.shapes;
create index on _tempus_import.shape_points using gist(geom);
create index on _tempus_import.shape_points(shape_id);

update tempus.pt_section set geom=t.geom from 
(select stop_from, stop_to, st_force3d(st_makeline(shape_points.geom order by shape_pt_sequence)) as geom
from
  _tempus_import.section_shape,
  _tempus_import.shape_points,
  tempus.pt_stop as s1,
  tempus.pt_stop as s2
where
  section_shape.shape_id = shape_points.shape_id and
  stop_from = s1.id and
  stop_to = s2.id and
  shape_pt_sequence between
    (select shape_pt_sequence from _tempus_import.shape_points where geom && s1.geom and shape_id = section_shape.shape_id order by shape_pt_sequence desc limit 1)
  and
    (select shape_pt_sequence from _tempus_import.shape_points where geom && s2.geom and shape_id = section_shape.shape_id order by shape_pt_sequence asc limit 1)  

group by stop_from, stop_to
) t
where pt_section.stop_from = t.stop_from and pt_section.stop_to = t.stop_to;

