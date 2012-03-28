/* ==== PT network ==== */
insert into
	tempus.pt_network
select
	-- FIXME : use serial instead ?
	, agency_id as id
	, agency_name as pnname
from
	_tempus_import.agency;
	
/* ==== Stops ==== */

-- add geometry index on stops import table
-- geometries should have been created in stops table 
-- during importation with 2154 srid from x, y latlon fields
-- st_transform(st_setsrid(st_point(stop_lon, stop_lat), 4326), 2154)
create or replace index _tempus_import.idx_stops_geom on _tempus_import.stops using gist(geom);

-- remove constraint on tempus stops
alter table tempus.pt_stop drop CONSTRAINT pt_stop_pkey;
alter table tempus.pt_stop drop CONSTRAINT pt_stop_parent_station_fkey;
alter table tempus.pt_stop drop CONSTRAINT enforce_dims_geom;
alter table tempus.pt_stop drop CONSTRAINT enforce_geotype_geom;
alter table tempus.pt_stop drop CONSTRAINT enforce_srid_geom;

-- insert data
insert into
	tempus.pt_stop
select 
	stop_id::integer as id
	, stop_name as psname
	, location_type::boolean as location_type
	, parent_station::integer as parent_station
	, road_section_id::bigint as road_section_id
	, zone_id::integer as zone_id
	-- abscissa_road_section is a float between 0 and 1
	, st_line_locate_point(geom_road, geom)::double precision as abscissa_road_section
	, geom
 from (
	select
		stops.*
		, first_value(rs.gid) over nearest as road_section_id
		, first_value(rs.geom) over nearest as geom_road
		, row_number() over nearest as nth
	from
		_tempus_import.stops
	join
		_tempus_import.road_section as rs
	on
		-- only consider road sections within xx meters
		-- stops further than this distance will not be included
		st_dwithin(stops.geom, rs.geom, 30)
	window
		-- select the nearest road geometry for each stop
		nearest as (partition by stops.id order by st_distance(stops.geom, rs.geom))
) as stops_ratt
where 
	-- only take one rattachement
	nth = 1;

-- restore constraints on pt_stop
alter table tempus.pt_stop add CONSTRAINT pt_stop_pkey PRIMARY KEY (id);
alter table tempus.pt_stop add CONSTRAINT pt_stop_parent_station_fkey FOREIGN KEY (parent_station)
	REFERENCES tempus.pt_stop (id) MATCH SIMPLE ON UPDATE NO ACTION ON DELETE NO ACTION;
alter table tempus.pt_stop add CONSTRAINT enforce_dims_geom CHECK (st_ndims(geom) = 2);
alter table tempus.pt_stop add CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geom) = 'POINT'::text OR geom IS NULL);
alter table tempus.pt_stop add CONSTRAINT enforce_srid_geom CHECK (st_srid(geom) = 2154);

/* ==== /stops ==== */


