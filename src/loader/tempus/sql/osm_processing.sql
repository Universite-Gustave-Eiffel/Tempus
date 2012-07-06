-- split ways on crossing points
--
-- 1. create a temp table of ways that are split on one of their crossing points
-- 2. update the way table, set the geom to the first part of the split
-- 3. insert a new way that is made of the second part of the split
--
-- must be repeated until no new row are added (ways with N crossing points are cut in N-1 executions)
drop schema if exists _processing cascade;
create schema _processing;

create table _processing.tmp as
(select wid, wgeom, node_from, node_to, pid, pgeom, st_split( wgeom, pgeom ) as split from
(
	select
	distinct on (wid) -- only select the first crossing point
		w.id as wid,
		w.geom as wgeom,
		w.node_from,
		w.node_to,
		p.id as pid,
		p.geom as pgeom
	from
		tempus.road_node as p
	join
		tempus.road_section as w
	on
		st_intersects( p.geom, w.geom )
	and
		p.id != w.node_from
	and
		p.id != w.node_to
) as t
);

-- update the geometry to the first split geometry
update tempus.road_section
set
	node_from = tmp.node_from,
	node_to = tmp.pid,
	geom = st_geometryN(tmp.split, 1)
from
	_processing.tmp as tmp
where
	id = tmp.wid;

-- insert the second split geometry
drop sequence if exists seq_section;
create temp sequence seq_section;
select setval('seq_section', (select max(id)+1 from tempus.road_section), false);

insert into
	tempus.road_section
select
	nextval('seq_section')::bigint as id,
	olds.road_type,
	tmp.pid as node_from,
	olds.node_to,
	olds.transport_type_ft,
	olds.transport_type_tf,
	olds.length,
	olds.car_speed_limit,
	olds.car_average_speed,
	olds.road_name,
	olds.lane,
	olds.roundabout,
	olds.bridge,
	olds.tunnel,
	olds.ramp,
	olds.tollway,
	st_geometryN(tmp.split,2) as geom
from
	tempus.road_section as olds
join
	_processing.tmp as tmp
on
	olds.id = tmp.wid;
	