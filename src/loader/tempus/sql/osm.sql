-- Tempus - OSM SQL import

-- update import table if needed

do $$
declare
  col text;
begin
  -- add bridge column if needed
  select into col column_name from information_schema.columns where table_name='highway' and table_schema='_tempus_import' and column_name='bridge';
  if col is null then
    alter table _tempus_import.highway add column bridge integer;
    update _tempus_import.highway set bridge = 0;
  end if;

  -- add tunnel column if needed
  select into col column_name from information_schema.columns where table_name='highway' and table_schema='_tempus_import' and column_name='tunnel';
  if col is null then
    alter table _tempus_import.highway add column tunnel integer;
    update _tempus_import.highway set tunnel = 0;
  end if;
end$$;

-- TABLE road_node

-- fill table
drop sequence if exists tempus.seq_road_node_id;
create sequence tempus.seq_road_node_id start with 1;

insert into 
	tempus.road_node
select
	nextval('tempus.seq_road_node_id')::bigint as id
	, false as bifurcation
	, st_force_3DZ(nodes.gnode) as geom
from (
	select 
		st_transform(st_startpoint(geom), 2154) as gnode
	from
		_tempus_import.highway
	union
	select
		st_transform(st_endpoint(geom), 2154) as gnode
	from
		_tempus_import.highway
) as nodes;

create index idx_road_node on tempus.road_node using gist(geom);


-- indexing
drop index if exists idx_road_node_geom;
create index idx_road_node_geom on tempus.road_node using gist(geom);

-- TABLE road_section

-- drop all constraints
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_node_from_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_node_to_fkey;
ALTER TABLE tempus.poi DROP CONSTRAINT poi_road_section_id_fkey;
ALTER TABLE tempus.pt_stop DROP CONSTRAINT pt_stop_road_section_id_fkey;
ALTER TABLE tempus.road_section_speed DROP CONSTRAINT road_section_speed_road_section_id_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_pkey;

-- id sequence
drop sequence if exists _tempus_import.seq_road_section_import ;
create sequence _tempus_import.seq_road_section_import start with 1;

create table _tempus_import.road_section_idmap AS
       SELECT nextval('_tempus_import.seq_road_section_import')::bigint as id, osm_id
              FROM _tempus_import.highway;
create index on _tempus_import.road_section_idmap(id);
create index on _tempus_import.road_section_idmap(osm_id);

-- insert into table
insert into
	tempus.road_section
select
        (SELECT id FROM _tempus_import.road_section_idmap WHERE osm_id = hw.osm_id ) as id
	, case
		-- FIXME : check type correspondance with
		-- http://wiki.openstreetmap.org/wiki/Routing
		when hw."type" = 'motorway' then 1
		when hw."type" = 'motorway_Link' then 1
		when hw."type" = 'trunk' then 1
		when hw."type" = 'trunk_Link' then 1
		when hw."type" = 'primary' then 2
		when hw."type" = 'primary_Link' then 2
		when hw."type" = 'secondary' then 3
		when hw."type" = 'tertiary' then 3
		when hw."type" = 'residental' then 4
		when hw."type" = 'service' then 5
		when hw."type" = 'track' then 5
		when hw."type" = 'unclassified' then 4
		else 4		
	end as road_type
	, nf.id as node_from
	, nt.id as node_to
	, case
		when hw."type" in ('motorway', 'motorway_Link', 'trunk', 'trunk_Link', 'primary', 'primary_Link') then 4+8+16+32 -- car + taxi + carpool + truck
                when hw."type" = 'cycleway' then 2 -- bicycle only
                when hw."type" = 'footway' then 1 -- pedestrian only
		else 32+16+8+4+2+1
	end as traffic_rules_ft
	, case
		when oneway::text in ('true', 'yes', '1') then
                  case when hw."type" in ('motorway', 'motorway_Link', 'trunk', 'trunk_Link', 'primary', 'primary_Link', 'cycleway') then 0 -- nothing here
                       else 1 -- only pedestrian is allowed
                  end
		else
                  case
                        when hw."type" in ('motorway', 'motorway_Link', 'trunk', 'trunk_Link', 'primary', 'primary_Link') then 4+8+16+32 -- car + taxi + carpool + truck
                        when hw."type" = 'cycleway' then 2 -- bicycle only
                        when hw."type" = 'footway' then 1 -- pedestrian only
		        else 32+16+8+4+2+1
                  end
	end as traffic_rules_tf
	, st_length(st_transform(hw.geom, 2154)) as length
	, case
		-- FIXME : check type correspondance with
		-- http://wiki.openstreetmap.org/wiki/Routing
		when hw."type" = 'motorway' then 130
		when hw."type" = 'motorway_Link' then 130
		when hw."type" = 'trunk' then 110
		when hw."type" = 'trunk_Link' then 110
		when hw."type" = 'primary' then 90
		when hw."type" = 'primary_Link' then 90
		when hw."type" = 'secondary' then 90
		when hw."type" = 'tertiary' then 90
		when hw."type" = 'residental' then 50
		when hw."type" = 'service' then 50
		when hw."type" = 'track' then 50
		when hw."type" = 'unclassified' then 50
		when hw."type" = 'footway' then 0
		else 50
	end as car_speed_limit	
	, hw."name" as road_name
	, hw.lanes as lane
	, false as roundabout
	, hw.bridge > 0 as bridge
	, hw.tunnel > 0 as tunnel
	, false as ramp
	, false as tollway
	, st_force_3DZ(st_transform(hw.geom, 2154)) as geom
from
	_tempus_import.highway as hw
join
	tempus.road_node as nf
on
	st_intersects(st_transform(st_startpoint(hw.geom), 2154), nf.geom)
join
	tempus.road_node as nt
on
	st_intersects(st_transform(st_endpoint(hw.geom), 2154), nt.geom)
;

-- Restore constraints and index
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_pkey
	PRIMARY KEY (id);
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_node_from_fkey 
	FOREIGN KEY (node_from) REFERENCES tempus.road_node ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_node_to_fkey
	FOREIGN KEY (node_to) REFERENCES tempus.road_node(id) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE tempus.poi ADD CONSTRAINT poi_road_section_id_fkey
	FOREIGN KEY (road_section_id) REFERENCES tempus.road_section ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE tempus.pt_stop ADD CONSTRAINT pt_stop_road_section_id_fkey
	FOREIGN KEY (road_section_id) REFERENCES tempus.road_section ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE tempus.road_section_speed ADD CONSTRAINT road_section_speed_road_section_id_fkey
	FOREIGN KEY (road_section_id) REFERENCES tempus.road_section ON DELETE CASCADE ON UPDATE CASCADE;

-- INDEXES
create index idx_road_section_geom on tempus.road_section using gist(geom);
-- FIXME : other indexes needed ?

-- add road restrictions
do $$
declare
  col text;
begin
  select into col column_name from information_schema.columns where table_name='restriction' and table_schema='_tempus_import' and column_name='from_id';
  if col is not null then
     raise notice 'Add road restrictions';
     insert into tempus.road_restriction
     select osm_id, array[(select id from _tempus_import.road_section_idmap where osm_id=from_id),
                          (select id from _tempus_import.road_section_idmap where osm_id=to_id)] from _tempus_import.restriction;
     insert into tempus.road_restriction_time_penalty (restriction_id, period_id, traffic_rules, time_value)
     select id,
            0,
            4+8+16+32+64, 'inf'::float from tempus.road_restriction;
  end if;
end$$;

