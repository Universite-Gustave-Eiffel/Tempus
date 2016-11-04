-- example of query used to elevate road node points
update tempus.road_node
set geom=geomz
from
(
select
	id, st_setsrid(st_makepoint(st_x(pt.geom), st_y(pt.geom), st_value( rast, pt.geom )), %(native_srid)) as geomz
from
	dem
join
	tempus.road_node as pt
on
	st_intersects( rast, geom )
) t
where t.id = road_node.id;

-- example of query used to elevate road sections
update tempus.road_section
set geom=geomz
from
(
with pts
as
(
select
	id, (st_dumppoints(rs.geom)).geom as geom
	from
	tempus.road_section as rs
)
select
	id,
	st_makeline( 
	st_setsrid(st_makepoint(st_x(geom), st_y(geom), st_value(rast, geom)), %(native_srid))
	) as geomz
from
	pts
join
	dem
on
	st_intersects(rast, geom)
group by
	id
) t
where t.id = road_section.id;
