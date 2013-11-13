
-- pt_stop_id_map
drop table if exists _tempus_import.pt_stop_idmap;
create table _tempus_import.pt_stop_idmap
(
        id serial primary key,
        vendor_id varchar
);
select setval('_tempus_import.pt_stop_idmap_id_seq',
       (select case when max(id) is null then 1 else max(id) end from tempus.pt_stop));
insert into _tempus_import.pt_stop_idmap (vendor_id) 
       select stop_id from _tempus_import.stops;
create index pt_stop_idmap_vendor_idx on _tempus_import.pt_stop_idmap(vendor_id);

-- pt_route_id_map
drop table if exists _tempus_import.pt_route_idmap;
create table _tempus_import.pt_route_idmap
(
        id serial primary key,
        vendor_id varchar
);
select setval('_tempus_import.pt_route_idmap_id_seq',
       (select case when max(id) is null then 1 else max(id) end from tempus.pt_route));
insert into _tempus_import.pt_route_idmap (vendor_id) 
       select route_id from _tempus_import.routes;
create index pt_route_idmap_vendor_idx on _tempus_import.pt_route_idmap(vendor_id);

-- pt_trip_id_map
drop table if exists _tempus_import.pt_trip_idmap;
create table _tempus_import.pt_trip_idmap
(
        id serial primary key,
        vendor_id varchar
);
select setval('_tempus_import.pt_trip_idmap_id_seq',
       (select case when max(id) is null then 1 else max(id) end from tempus.pt_trip));
insert into _tempus_import.pt_trip_idmap (vendor_id) 
       select trip_id from _tempus_import.trips;
create index pt_trip_idmap_vendor_idx on _tempus_import.pt_trip_idmap(vendor_id);

-- pt_calendar_id_map
drop table if exists _tempus_import.pt_calendar_idmap;
create table _tempus_import.pt_calendar_idmap
(
        id serial primary key,
        vendor_id varchar
);
select setval('_tempus_import.pt_calendar_idmap_id_seq',
       (select case when max(id) is null then 1 else max(id) end from tempus.pt_service));
insert into _tempus_import.pt_calendar_idmap (vendor_id) 
       select service_id from _tempus_import.calendar
       -- add service ids from calendar_dates.txt
       union
       select service_id from _tempus_import.calendar_dates;       
create index pt_calendar_idmap_vendor_idx on _tempus_import.pt_calendar_idmap(vendor_id);

-- pt_fare_id_map
drop table if exists _tempus_import.pt_fare_idmap;
create table _tempus_import.pt_fare_idmap
(
        id serial primary key,
        vendor_id varchar
);
select setval('_tempus_import.pt_fare_idmap_id_seq',
       (select case when max(id) is null then 1 else max(id) end from tempus.pt_fare_attribute));
insert into _tempus_import.pt_fare_idmap (vendor_id) 
       select fare_id from _tempus_import.fare_attributes;
create index pt_fare_idmap_vendor_idx on _tempus_import.pt_fare_idmap(vendor_id);

/* ==== PT network ==== */
insert into
	tempus.pt_network (pnname, provided_transport_types)
select
	agency_name as pnname,
	56 as provided_transport_types -- bus + metro + tramway
from
	_tempus_import.agency;
	
/* ==== Stops ==== */

-- add geometry index on stops import table
-- geometries should have been created in stops table 
-- during importation with 2154 srid from x, y latlon fields
-- st_transform(st_setsrid(st_point(stop_lon, stop_lat), 4326), 2154)
create table 
	_tempus_import.stops_geom as 
select
	stop_id
	, st_force_3DZ(st_transform(st_setsrid(st_point(stop_lon, stop_lat), 4326), 2154)) as geom
from
	_tempus_import.stops;

drop index if exists _tempus_import.idx_stops_geom;
create index idx_stops_geom on _tempus_import.stops_geom using gist(geom);
drop index if exists _tempus_import.idx_stops_stop_id;
create index idx_stops_stop_id on _tempus_import.stops (stop_id);
drop index if exists _tempus_import.idx_stops_geom_stop_id;
create index idx_stops_geom_stop_id on _tempus_import.stops_geom (stop_id);

-- remove constraint on tempus stops and dependencies
alter table tempus.pt_stop drop CONSTRAINT pt_stop_parent_station_fkey;
alter table tempus.pt_stop drop CONSTRAINT pt_stop_road_section_id_fkey;
alter table tempus.pt_section drop constraint pt_section_stop_from_fkey;
alter table tempus.pt_section drop constraint pt_section_stop_to_fkey;
alter table tempus.pt_transfer drop constraint pt_transfer_from_stop_id_fkey;
alter table tempus.pt_transfer drop constraint pt_transfer_to_stop_id_fkey;
alter table tempus.pt_stop_time drop constraint pt_stop_time_stop_id_fkey;
alter table tempus.pt_stop drop CONSTRAINT pt_stop_pkey;

insert into
	tempus.pt_stop
select 
	(select id from _tempus_import.pt_stop_idmap where vendor_id=stop_id) as id,
        stop_id as vendor_id
	, stop_name as psname
	, location_type::boolean as location_type
        , (select id from _tempus_import.pt_stop_idmap where vendor_id=parent_station) as parent_station
	, road_section_id::bigint as road_section_id
	, zone_id::integer as zone_id
	-- abscissa_road_section is a float between 0 and 1
	, st_line_locate_point(geom_road, geom)::double precision as abscissa_road_section
	, geom
 from (
	select
		stops.*
		, stops_geom.geom as geom
		, first_value(rs.id) over nearest as road_section_id
		, first_value(rs.geom) over nearest as geom_road
		, row_number() over nearest as nth
	from
		_tempus_import.stops
	join
		_tempus_import.stops_geom
	on
		stops.stop_id = stops_geom.stop_id
	join
		tempus.road_section as rs
	on
		-- only consider road sections within xx meters
		-- stops further than this distance will not be included
		st_dwithin(stops_geom.geom, rs.geom, 100)
	window
		-- select the nearest road geometry for each stop
		nearest as (partition by stops.stop_id order by st_distance(stops_geom.geom, rs.geom))
) as stops_ratt
where 
	-- only take one rattachement
	nth = 1;

-- set parent_station to null when the parent_station is not present (out of road network scope ?)
update tempus.pt_stop
set parent_station=null
where id in
      (
      select
        s1.id
      from
        tempus.pt_stop as s1
      left join
        tempus.pt_stop as s2
      on s1.parent_station = s2.id
      where
        s1.parent_station is not null
        and s2.id is null
);

-- restore constraints on pt_stop
alter table tempus.pt_stop add CONSTRAINT pt_stop_pkey PRIMARY KEY (id);
alter table tempus.pt_stop add CONSTRAINT pt_stop_parent_station_fkey FOREIGN KEY (parent_station)
	REFERENCES tempus.pt_stop (id) MATCH SIMPLE ON UPDATE NO ACTION ON DELETE NO ACTION;
alter table tempus.pt_stop add CONSTRAINT pt_stop_road_section_id_fkey FOREIGN KEY (road_section_id)
      REFERENCES tempus.road_section (id);

/* ==== /stops ==== */

/* ==== GTFS routes ==== */
-- drop constraints 
-- TODO
insert into
	tempus.pt_route
select
	(select id from _tempus_import.pt_route_idmap where vendor_id=route_id) as id
        , route_id as vendor_id
	, (select id from tempus.pt_network as pn order by import_date desc limit 1) as network_id
	, route_short_name as short_name
	, route_long_name as long_name
	, route_type::integer as route_type
from
	_tempus_import.routes;

-- restore constraints

/* ==== sections ==== */
-- drop constraints
-- TODO

insert into tempus.pt_section (stop_from, stop_to, network_id, geom)
with trips as (
	select
		trip_id
		-- gtfs stop sequences can have holes
		-- use the dense rank to have then continuous
		, dense_rank() over win as stopseq
		, stop_sequence
		, stop_id
	from
		_tempus_import.stop_times
	window win as (
		partition by trip_id order by stop_sequence
	)
)
select
	stop_from
	, stop_to
	, (select id from tempus.pt_network as pn order by import_date desc limit 1) as network_id
	-- Geometry is a line between stops
	-- FIXME : if we have a shape.txt, could be a full shape
	, st_force_3DZ(st_setsrid(st_makeline(g1.geom, g2.geom), 2154)) as geom
from (
	select 
		(select id from _tempus_import.pt_stop_idmap where vendor_id=t1.stop_id) as stop_from,
		(select id from _tempus_import.pt_stop_idmap where vendor_id=t2.stop_id) as stop_to
	from 
		trips as t1
	join 
		trips as t2
	on 
		t1.trip_id = t2.trip_id 
		and t1.stopseq = t2.stopseq - 1 
	group by 
		t1.stop_id, t2.stop_id
) as foo
join
	-- get the from stop geometry
	tempus.pt_stop as g1
on
	stop_from = g1.id
join
	-- get the to stop geometry
	tempus.pt_stop as g2
on
	stop_to = g2.id;

-- restore constraints
alter table tempus.pt_section add constraint pt_section_stop_from_fkey FOREIGN KEY (stop_from)
      REFERENCES tempus.pt_stop(id);
alter table tempus.pt_section add constraint pt_section_stop_to_fkey FOREIGN KEY (stop_to)
      REFERENCES tempus.pt_stop(id);

/* ==== GTFS calendar ==== */

insert into
       tempus.pt_service(id, vendor_id)
select
       id, vendor_id
from
        _tempus_import.pt_calendar_idmap;

insert into
	tempus.pt_calendar
select
	(select id from _tempus_import.pt_calendar_idmap where vendor_id=service_id) as service_id
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
	tempus.pt_trip(id, vendor_id, route_id, service_id, short_name)
select
	(select id from _tempus_import.pt_trip_idmap where vendor_id=trip_id)
        , trip_id
	, (select id from _tempus_import.pt_route_idmap where vendor_id=route_id)
	, (select id from _tempus_import.pt_calendar_idmap where vendor_id=service_id)
	, trip_short_name
from
	_tempus_import.trips;

-- restore constraints

insert into
	tempus.pt_calendar_date(service_id, calendar_date, exception_type)
select
        (select id from _tempus_import.pt_calendar_idmap where vendor_id=service_id) as service_id
	, "date"::date as calendar_date
	, exception_type::integer as exception_type
from
	_tempus_import.calendar_dates;

-- restore constraints

-- convert GTFS time to regular time
-- GTFS time may be greater than 24
-- we apply here a modulo 24
-- when arrival is < departure, it means the arrival occurs the next day
CREATE OR REPLACE FUNCTION _tempus_import.format_gtfs_time(text)
RETURNS time without time zone AS $$
BEGIN
        IF substr($1,1,2)::integer > 23 THEN
           RETURN ((substr($1,1,2)::integer % 24) || substr($1,3,6))::time without time zone;
        ELSE
           RETURN $1::time without time zone;
        END IF;
END;
$$ LANGUAGE plpgsql;


insert into
	tempus.pt_stop_time (trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign
	, pickup_type, drop_off_type, shape_dist_traveled)
select
	(select id from _tempus_import.pt_trip_idmap where vendor_id=trip_id) as trip_id
	, _tempus_import.format_gtfs_time(arrival_time) as arrival_time
	, _tempus_import.format_gtfs_time(departure_time) as departure_time
	, (select id from _tempus_import.pt_stop_idmap where vendor_id=stop_id) as stop_id
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
	(select id from _tempus_import.pt_fare_idmap where vendor_id=fare_id) as fare_id
        , fare_id as vendor_id
	, price::double precision as price
	, currency_type::char(3) as currency_type
	-- FIXME : same in tempus than GTFS ?
	, transfers::integer as transfers
	, transfer_duration::integer as transfer_duration
from
	_tempus_import.fare_attributes;

insert into
	tempus.pt_frequency (trip_id, start_time, end_time, headway_secs)
select
	(select id from _tempus_import.pt_trip_idmap where vendor_id=trip_id) as trip_id
	, _tempus_import.format_gtfs_time(start_time) as start_time
	, _tempus_import.format_gtfs_time(end_time) as end_time
	, headway_secs::integer as headway_secs
from
	_tempus_import.frequencies;


insert into
	tempus.pt_fare_rule (fare_id, route_id, origin_id, destination_id, contains_id)
select
	(select id from _tempus_import.pt_fare_idmap where vendor_id=fare_id) as fare_id
	, (select id from _tempus_import.pt_route_idmap where vendor_id=route_id) as route_id
	, (select id from _tempus_import.pt_stop_idmap where vendor_id=origin_id) as origin_id
	, (select id from _tempus_import.pt_stop_idmap where vendor_id=destination_id) as destination_id
	, (select id from _tempus_import.pt_stop_idmap where vendor_id=contains_id) as contains_id
from
	_tempus_import.fare_rules;

insert into
	tempus.pt_transfer
select
	(select id from _tempus_import.pt_stop_idmap where vendor_id=from_stop_id) as from_stop_id
	, (select id from _tempus_import.pt_stop_idmap where vendor_id=to_stop_id) as to_stop_id
	, transfer_type::integer as transfer_type
	, min_transfer_time::integer as min_transfer_time
from
	_tempus_import.transfers;

alter table tempus.pt_transfer add constraint pt_transfer_from_stop_id_fkey FOREIGN KEY (from_stop_id)
      REFERENCES tempus.pt_stop (id);
alter table tempus.pt_transfer add constraint pt_transfer_to_stop_id_fkey FOREIGN KEY (to_stop_id)
      REFERENCES tempus.pt_stop (id);
alter table tempus.pt_stop_time add constraint pt_stop_time_stop_id_fkey FOREIGN KEY (stop_id)
      REFERENCES tempus.pt_stop (id);
