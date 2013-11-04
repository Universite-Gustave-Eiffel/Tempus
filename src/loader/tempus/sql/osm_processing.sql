drop sequence if exists seq_section;
create temp sequence seq_section;
select setval('seq_section', (select max(id)+1 from tempus.road_section), false);

drop table if exists road_section_tmp;
create temporary table road_section_tmp as
(

with recursive split(section_id, lines, array_pid, array_pts)
as
(


with points as
(
select
	sid, st_multi(sgeom) as array_lines, array_agg(pid) as array_pid, array_agg(pgeom) as array_pts
from
(
select s.id as sid, p.id as pid, p.geom as pgeom, s.geom as sgeom
	from
		tempus.road_node as p
	join
		tempus.road_section as s -- sections declared below
	on
		st_intersects( p.geom, s.geom )
	and
		p.id != s.node_from
	and
		p.id != s.node_to
        -- if two nodes (with different ids) share the same coordinates
	and
		not st_intersects( p.geom, st_startpoint(s.geom))
	and
		not st_intersects( p.geom, st_endpoint(s.geom))
) t
group by
	sid, sgeom

)
select * from points

union all

select
	section_id,
	st_split(lines, array_pts[1]),
	array_pid,
	array_pts[2:array_length(array_pts,1)]
from
	split
where
	array_length(array_pts,1) is not null
),
t as
(
	with recursive assign_nodes(section_id,line, array_lines, i, n, array_from, array_to) as
	(
		select
			section_id, lines, array[]::geometry[], 0, st_numgeometries(lines), array[]::bigint[], array[]::bigint[]
		from
			split
		where
			-- selection of the most "splitted" line
			array_length(array_pts, 1) is null
		union all

		select
			section_id,
			line,
			array_lines || st_geometryn(line,i+1),
			i+1,
			n,
			array_from || tnode_from.id,
			array_to || tnode_to.id
		from
			assign_nodes,
			tempus.road_node as tnode_from,
			tempus.road_node as tnode_to
		where
			st_intersects(tnode_from.geom, st_startpoint(st_geometryn(line,i+1)))
		and
			st_intersects(tnode_to.geom, st_endpoint(st_geometryn(line,i+1)))
		and
			-- recursion stop
			i < n
	)
	select section_id, array_lines, array_from, array_to, n from assign_nodes
	where i = n
)
select
	t.section_id,
	nextval('seq_section')::bigint as id,
	rs.road_type,
	array_from[i] as node_from,
	array_to[i] as node_to,
	rs.transport_type_ft,
	rs.transport_type_tf,
	st_length(array_lines[i]) as length,
	rs.car_speed_limit,
	rs.car_average_speed,
	rs.road_name,
	rs.lane,
	rs.roundabout,
	rs.bridge,
	rs.tunnel,
	rs.ramp,
	rs.tollway,
	array_lines[i] as geom
from
	t,
	(select generate_series(1,n) as i, section_id from t) as tt,
	tempus.road_section as rs
where
	tt.section_id = t.section_id
and
	rs.id = t.section_id

)
;

-- delete sections that will be replaced by a sequence of sections
delete from tempus.road_section where id in (select section_id from road_section_tmp);

-- insert new sections
insert into tempus.road_section
select
	-- all columns except section_id
	id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length,
	car_speed_limit, car_average_speed, road_name, lane, roundabout, bridge, tunnel, ramp, tollway, geom
from
	road_section_tmp
;
