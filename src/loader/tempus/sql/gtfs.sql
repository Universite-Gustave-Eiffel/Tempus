/* ==== PT network ==== */
insert into
	tempus.pt_network (pnname)
select
	agency_name as pnname
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

/* ==== GTFS routes ==== */
-- drop constraints 
-- TODO

insert into
	tempus.pt_route
select
	route_id::integer as id
	, agency_id::integer as network_id
	, route_short_name as short_name
	, route_long_name as long_name
	, route_type::integer as route_type
from
	_tempus_import.routes;

-- restore constraints

/* ==== sections ==== */
-- drop constraints
-- TODO

insert into
	tempus.pt_section
select
	*
from
	foo;
-- TODO
-- restore constraints

/* ==== GTFS calendar ==== */
insert into
	tempus.pt_calendar
select
	service_id::integer as id
	, monday::boolean as monday
	, tuesday::boolean as tuesday
	, wednesday::boolean as wednesday
	, thursday::boolean as thursday
	, friday::boolean as friday
	, saturday::boolean as saturday
	, sunday::boolean as sunday
	, start_date::date as start_date
	, end_date::date as end_date
from
	_tempus_import.calendar;

insert into 
	tempus.pt_trip
select
	trip_id::bigint as id
	, route_id::integer as route_id
	, service_id::integer as service_id
	, trip_short_name
from
	_tempus_import.trips;

-- restore constraints

insert into
	tempus.pt_calendar_date
select
	service_id::bigint as service_id
	, "date"::date as calendar_date
	, exception_type::integer as exception_type
from
	_tempus_import.calendar_dates;

-- restore constraints

insert into
	tempus.pt_stop_time
select
	-- id ??
	-- serial ?
	nextval('seq_pt_stop_time_id') as id
	, trip_id::bigint as trip_id
	, arrival_time::time without time zone as arrival_time
	, departure_time::time without time zone as departure_time
	, stop_id::integer as stop_id
	, stop_sequence::integer as stop_sequence
	, stop_headsign
	, pickup_type::integer as pickup_type
	, drop_off_type::integer as drop_off_type
	, shape_dist_traveled::double precision as shape_dist_traveled
from
	_tempus_import.stop_times;

-- restore constraints

insert into
	tempus.pt_fare_attribute
select
	fare_id::integer as id
	, price::double precision as price
	, currency_type::char(3) as currency_type
	-- FIXME : same in tempus than GTFS ?
	, transfers::integer as transfers
	, transfer_duration::integer as transfer_duration
from
	_tempus_import.fare_attributes;

insert into
	tempus.pt_frequency
select
	-- id : serial ?
	nextval('seq_pt_frequency_id') as id
	, trip_id::bigint as trip_id
	, start_time::time without time zone as start_time
	, end_time::time without time zone as end_time
	, headways_secs::integer as headways_secs
from
	_tempus_import.frequencies;


insert into
	tempus.pt_fare_rule
select
	nextval('seq_pt_fare_rule_id')::bigint as id
	, fare_id::bigint as fare_id
	, route_id::bigint as route_id
	, origin_id::integer as origin_id
	, destination_id::integer as destination_id
	, contains_id::integer as contains_id
from
	_tempus_import.fare_rules;

-- TODO : add transfer table into import
insert into
	tempus.pt_transfer
select
	from_stop_id::integer as from_stop_id
	, to_stop_id::integer as to_stop_id
	, transfer_type::integer as transfer_type
	, min_transfer_time::integer as min_transfer_time
from
	_tempus_import.transfers;