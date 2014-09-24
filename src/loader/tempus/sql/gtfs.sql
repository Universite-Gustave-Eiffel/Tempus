do $$
begin
raise notice '==== Init ===';
end$$;
-- pt_stop_id_map
drop table if exists _tempus_import.pt_stop_idmap;
create table _tempus_import.pt_stop_idmap
(
        id serial primary key,
        vendor_id varchar
);

select setval('_tempus_import.pt_stop_idmap_id_seq', (select case when max(id) is null then 1 else max(id)+1 end from tempus.pt_stop), false);
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
select setval('_tempus_import.pt_route_idmap_id_seq', (select case when max(id) is null then 1 else max(id)+1 end from tempus.pt_route), false);
insert into _tempus_import.pt_route_idmap (vendor_id) 
       select route_id from _tempus_import.routes;
create index pt_route_idmap_vendor_idx on _tempus_import.pt_route_idmap(vendor_id);

-- pt_agency_id_map
drop table if exists _tempus_import.pt_agency_idmap; 
create table _tempus_import.pt_agency_idmap
(
	id serial primary key, 
	vendor_id varchar
); 
select setval('_tempus_import.pt_agency_idmap_id_seq', (select case when max(id) is null then 1 else max(id)+1 end from tempus.pt_agency), false); 
insert into _tempus_import.pt_agency_idmap(vendor_id)
	select agency_id from _tempus_import.agency; 
	create index pt_agency_idmap_vendor_idx on _tempus_import.pt_agency_idmap(vendor_id); 

-- pt_trip_id_map
drop table if exists _tempus_import.pt_trip_idmap;
create table _tempus_import.pt_trip_idmap
(
        id serial primary key,
        vendor_id varchar
);
select setval('_tempus_import.pt_trip_idmap_id_seq', (select case when max(id) is null then 1 else max(id)+1 end from tempus.pt_trip), false);
insert into _tempus_import.pt_trip_idmap (vendor_id) 
       select trip_id from _tempus_import.trips;
create index pt_trip_idmap_vendor_idx on _tempus_import.pt_trip_idmap(vendor_id);

-- pt_service_id_map
drop table if exists _tempus_import.pt_service_idmap; 
create table _tempus_import.pt_service_idmap
(
	id serial primary key, 
	vendor_id varchar
); 
select setval('_tempus_import.pt_service_idmap_id_seq', (select case when max(service_id) is null then 1 else max(service_id)+1 end from ((select service_id from tempus.pt_calendar) union (select service_id from tempus.pt_calendar_date)) r ), false); 
insert into _tempus_import.pt_service_idmap( vendor_id )
	select service_id from _tempus_import.calendar union select service_id from _tempus_import.calendar_dates; 
create index pt_service_idmap_vendor_idx on _tempus_import.pt_service_idmap(vendor_id);

-- pt_fare_id_map
drop table if exists _tempus_import.pt_fare_idmap;
create table _tempus_import.pt_fare_idmap
(
        id serial primary key,
        vendor_id varchar
);
select setval('_tempus_import.pt_fare_idmap_id_seq', (select case when max(id) is null then 1 else max(id)+1 end from tempus.pt_fare_attribute));
insert into _tempus_import.pt_fare_idmap (vendor_id) 
       select fare_id from _tempus_import.fare_attributes;
create index pt_fare_idmap_vendor_idx on _tempus_import.pt_fare_idmap(vendor_id);

do $$
begin
raise notice '==== PT network ====';
end$$;
/* ==== PT network ==== */
-- insert a new network (with null name for now) FIXME get the name from the command line
insert into tempus.pt_network(pnname) 
select '%(network)'; 

insert into
	tempus.pt_agency (id, paname, network_id)
select
	(select id from _tempus_import.pt_agency_idmap where vendor_id=agency_id) as id,
	agency_name, 
	(select id from tempus.pt_network as pn order by import_date desc limit 1) as network_id
from
	_tempus_import.agency;
	
/* ==== Stops ==== */

-- add geometry index on stops import table
-- geometries should have been created in stops table 
-- during importation with 2154 srid from x, y latlon fields
-- st_transform(st_setsrid(st_point(stop_lon, stop_lat), 4326), 2154)
drop table if exists _tempus_import.stops_geom;
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
alter table tempus.pt_stop drop CONSTRAINT pt_stop_road_section_id_fkey;
alter table tempus.pt_stop drop CONSTRAINT pt_stop_parent_station_fkey; 
alter table tempus.pt_transfer drop constraint pt_transfer_from_stop_id_fkey;
alter table tempus.pt_transfer drop constraint pt_transfer_to_stop_id_fkey;
alter table tempus.pt_stop_time drop constraint pt_stop_time_stop_id_fkey;
alter table tempus.pt_section drop constraint if exists pt_section_stop_from_fkey;
alter table tempus.pt_section drop constraint if exists pt_section_stop_to_fkey;

do $$
begin
raise notice '==== Insert new road nodes if needed ====';
end$$;
/* ==== Stops ==== */

drop sequence if exists tempus.seq_road_node_id;
create sequence tempus.seq_road_node_id start with 1;
select setval('tempus.seq_road_node_id', (select max(id) from tempus.road_node));

drop table if exists _tempus_new_nodes;
create /*temporary*/ table _tempus_new_nodes as
select stop_id, nextval('tempus.seq_road_node_id')::bigint as node_id
from
(
select
		distinct stops.stop_id
	from
		_tempus_import.stops
	join
		_tempus_import.stops_geom
	on
		stops.stop_id = stops_geom.stop_id
	left join
		tempus.road_section as rs
	on
		-- only consider road sections within xx meters
		-- stops further than this distance will not be included
		st_dwithin(stops_geom.geom, rs.geom, 500)
	where
	     rs.id is null
) as t;

insert into tempus.road_node
select 
   nn.node_id as id,
   false as bifurcation,
   st_force3DZ( geom ) as geom
   from _tempus_new_nodes as nn,
        _tempus_import.stops_geom as sg
   where
        nn.stop_id = sg.stop_id;

drop sequence if exists tempus.seq_road_section_id;
create sequence tempus.seq_road_section_id start with 1;
select setval('tempus.seq_road_section_id', (select max(id) from tempus.road_section));

insert into tempus.road_section
select 
   nextval('tempus.seq_road_section_id')::bigint as id, 
   1 as road_type, -- ??
   node_id as node_from,
   node_id as node_to,
   65535 as traffic_rules_ft,
   65535 as traffic_rules_tf,
   0 as length,
   0 as car_speed_limit,
   '' as road_name,
   1 as lane,
   false as roundabout,
   false as bridge,
   false as tunnel,
   false as ramp,
   false as tollway,
   -- create an artificial line around the stop
   st_makeline(st_translate(geom, -10, 0, 0),st_translate(geom,10,0,0)) as geom
from
   _tempus_new_nodes as nn,
   _tempus_import.stops_geom as sg
where nn.stop_id = sg.stop_id
;


do $$
begin
raise notice '==== PT stops ====';
end$$;

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
		st_dwithin(stops_geom.geom, rs.geom, 500)
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
alter table tempus.pt_stop add CONSTRAINT pt_stop_road_section_id_fkey FOREIGN KEY (road_section_id)
      REFERENCES tempus.road_section (id);
alter table tempus.pt_stop add CONSTRAINT pt_stop_parent_station_fkey FOREIGN KEY (parent_station)
	REFERENCES tempus.pt_stop (id) MATCH SIMPLE ON UPDATE CASCADE ON DELETE CASCADE;

/* ==== /stops ==== */

do $$
begin
raise notice '==== Transport modes ====';
end$$;

CREATE TABLE _tempus_import.transport_mode
(
  id serial NOT NULL,
  name character varying, -- Description of the mode
  public_transport boolean NOT NULL,
  gtfs_route_type integer -- Reference to the equivalent GTFS code (for PT only)
); 

INSERT INTO _tempus_import.transport_mode(name, public_transport, gtfs_route_type)
SELECT
	case
        when r.route_type = 0 then 'Tram (%(network))'
        when r.route_type = 1 then 'Subway (%(network))'
        when r.route_type = 2 then 'Train (%(network))'
        when r.route_type = 3 then 'Bus (%(network))'
        when r.route_type = 4 then 'Ferry (%(network))'
        when r.route_type = 5 then 'Cable-car (%(network))'
        when r.route_type = 6 then 'Suspended Cable-Car (%(network))'
        when r.route_type = 7 then 'Funicular (%(network))'
        end, 
	TRUE, 
	r.route_type
FROM (SELECT DISTINCT route_type FROM _tempus_import.routes) r
;

INSERT INTO tempus.transport_mode(name, public_transport, gtfs_route_type)
SELECT 
	name, public_transport, gtfs_route_type
FROM _tempus_import.transport_mode; 

do $$
begin
raise notice '==== PT routes ====';
end$$;
/* ==== GTFS routes ==== */
-- drop constraints 
alter table tempus.pt_route drop CONSTRAINT pt_route_agency_id_fkey;

insert into tempus.pt_route( id, vendor_id, short_name, long_name, transport_mode, agency_id )
select * from 
(select
	(select id from _tempus_import.pt_route_idmap where vendor_id=route_id) as id
        , route_id as vendor_id
	, route_short_name as short_name
	, route_long_name as long_name
	, transport_mode.id
        -- use the agency_id if available, else set to the only one in agency.txt
        , (select id from _tempus_import.pt_agency_idmap where vendor_id = (case when agency_id is null then (select agency_id from _tempus_import.agency) else agency_id end) ) as agency_id
from
	_tempus_import.routes, tempus.transport_mode
	where transport_mode.gtfs_route_type = routes.route_type::integer
) q;

-- restore constraints
alter table tempus.pt_route add CONSTRAINT pt_route_agency_id_fkey FOREIGN KEY (agency_id)
	REFERENCES tempus.pt_agency (id) MATCH SIMPLE ON UPDATE CASCADE ON DELETE CASCADE;

do $$
begin
raise notice '==== PT sections ====';
end$$;
/* ==== sections ==== */
-- drop constraints
alter table tempus.pt_section drop constraint if exists pt_section_network_id_fkey;;

insert into tempus.pt_section (stop_from, stop_to, network_id, geom)
with pt_seq as (
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

        distinct foo.stop_from
        , foo.stop_to
        , (select id from tempus.pt_network as pn order by import_date desc limit 1) as network_id
        -- Geometry is a line between stops
        -- FIXME : if we have a shape.txt, could be a full shape
        , st_force_3DZ(st_setsrid(st_makeline(g1.geom, g2.geom), 2154)) as geom
from (
        select
                (select id from _tempus_import.pt_stop_idmap where vendor_id=t1.stop_id) as stop_from,
                (select id from _tempus_import.pt_stop_idmap where vendor_id=t2.stop_id) as stop_to,
                routes.route_type
        from
                pt_seq as t1
        join
                pt_seq as t2
        on
                t1.trip_id = t2.trip_id
                and t1.stopseq = t2.stopseq - 1
        join
                _tempus_import.trips
        on
                t1.trip_id = trips.trip_id
        join
                _tempus_import.routes
        on
                trips.route_id = routes.route_id
        group by
            t1.stop_id, t2.stop_id, route_type
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
        stop_to = g2.id ;

-- restore constraints
alter table tempus.pt_section add constraint pt_section_stop_from_fkey FOREIGN KEY (stop_from)
      REFERENCES tempus.pt_stop(id) MATCH SIMPLE ON UPDATE CASCADE ON DELETE CASCADE;
alter table tempus.pt_section add constraint pt_section_stop_to_fkey FOREIGN KEY (stop_to)
      REFERENCES tempus.pt_stop(id) MATCH SIMPLE ON UPDATE CASCADE ON DELETE CASCADE;


/* ==== GTFS calendar ==== */

do $$
begin
raise notice '==== PT calendar ====';
end$$;
insert into tempus.pt_calendar
select
	(select id from _tempus_import.pt_service_idmap where vendor_id=service_id) as service_id
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


do $$
begin
raise notice '==== PT trips ====';
end$$;
-- drop constraints
alter table tempus.pt_trip drop constraint if exists pt_trip_route_id_fkey;; 

insert into tempus.pt_trip(id, vendor_id, route_id, service_id, short_name)
	select * from
	(
	select
		(select id from _tempus_import.pt_trip_idmap where vendor_id=trip_id)
		, trip_id
		, (select id from _tempus_import.pt_route_idmap where vendor_id=route_id) as rid
		, (select id from _tempus_import.pt_service_idmap where vendor_id=service_id) as sid
		, trip_short_name
	from
		_tempus_import.trips) q
	where sid is not null and rid is not null;

-- restore constraints
alter table tempus.pt_trip add constraint pt_trip_route_id_fkey FOREIGN KEY(route_id)
	REFERENCES tempus.pt_route(id) MATCH SIMPLE ON UPDATE CASCADE ON DELETE CASCADE;; 

do $$
begin
raise notice '==== PT calendar dates ====';
end$$;

insert into
	tempus.pt_calendar_date(service_id, calendar_date, exception_type)
select
        (select id from _tempus_import.pt_service_idmap where vendor_id=service_id) as service_id
	, "date"::date as calendar_date
	, exception_type::integer as exception_type
from
	_tempus_import.calendar_dates;


do $$
begin
raise notice '==== PT stop times ====';
end$$;

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
select * from
(
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
		_tempus_import.stop_times) q
where trip_id in (select id from tempus.pt_trip) and stop_id in (select id from tempus.pt_stop);

-- restore constraints


do $$
begin
raise notice '==== Update PT networks ====';
end$$;

-- Update network begin and end calendar dates
UPDATE tempus.pt_network
SET calendar_begin = req.min, calendar_end = req.max
FROM 
(
	WITH req_min AS (
	SELECT service_id, min(calendar_date)
	FROM
	(
	SELECT service_id, start_date as calendar_date FROM tempus.pt_calendar
	UNION
	SELECT service_id, calendar_date FROM tempus.pt_calendar_date
	) r
	GROUP BY service_id
	ORDER BY service_id
	), 
	req_max AS (
	SELECT service_id, max(calendar_date)
	FROM
	(
	SELECT service_id, start_date as calendar_date FROM tempus.pt_calendar
	UNION
	SELECT service_id, calendar_date FROM tempus.pt_calendar_date
	) r
	GROUP BY service_id
	ORDER BY service_id
	)
	SELECT pt_agency.network_id, min(req_min.min), max(req_max.max)
	  FROM tempus.pt_trip, tempus.pt_route, tempus.pt_agency, req_min, req_max
	  WHERE pt_agency.id = pt_route.agency_id AND pt_route.id = pt_trip.route_id AND pt_trip.service_id = req_min.service_id AND pt_trip.service_id = req_max.service_id 
	  GROUP BY pt_agency.network_id
	  ORDER BY network_id
) req
WHERE req.network_id = pt_network.id; 


-- Update transport modes desserving pt stops
--UPDATE tempus.pt_stop
--SET transport_mode = reqB.transport_mode
--FROM 
--(
--SELECT stop_id, sum(transport_mode) AS transport_mode
--FROM
--(
--SELECT distinct pt_stop_time.stop_id, pt_route.transport_mode
--FROM tempus.pt_stop_time, tempus.pt_trip, tempus.pt_route
--WHERE pt_stop_time.trip_id = pt_trip.id AND pt_trip.route_id = pt_route.id
--) reqA
--GROUP BY stop_id
--) reqB
--WHERE pt_stop.id = reqB.stop_id ; 

do $$
begin
raise notice '==== PT fare attribute ====';
end$$;

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

do $$
begin
raise notice '==== PT frequency ====';
end$$;
insert into
	tempus.pt_frequency (trip_id, start_time, end_time, headway_secs)
select
	(select id from _tempus_import.pt_trip_idmap where vendor_id=trip_id) as trip_id
	, _tempus_import.format_gtfs_time(start_time) as start_time
	, _tempus_import.format_gtfs_time(end_time) as end_time
	, headway_secs::integer as headway_secs
from
	_tempus_import.frequencies;


do $$
begin
raise notice '==== PT fare rule ====';
end$$;
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

do $$
begin
raise notice '==== PT transfer ====';
end$$;

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
      REFERENCES tempus.pt_stop (id) ON UPDATE CASCADE ON DELETE CASCADE;
alter table tempus.pt_transfer add constraint pt_transfer_to_stop_id_fkey FOREIGN KEY (to_stop_id)
      REFERENCES tempus.pt_stop (id)  ON UPDATE CASCADE ON DELETE CASCADE;
alter table tempus.pt_stop_time add constraint pt_stop_time_stop_id_fkey FOREIGN KEY (stop_id)
      REFERENCES tempus.pt_stop (id)  ON UPDATE CASCADE ON DELETE CASCADE;


do $$
begin
raise notice '==== Adding views and cleaning data';
end
$$;

CREATE OR REPLACE VIEW tempus.pt_stop_by_network AS
 SELECT DISTINCT pt_stop.id AS stop_id, pt_stop.vendor_id, pt_stop.name, pt_stop.location_type, pt_stop.parent_station, 
 pt_route.transport_mode, pt_stop.road_section_id, pt_stop.zone_id, pt_stop.abscissa_road_section, pt_stop.geom, pt_network.id AS network_id
   FROM tempus.pt_stop, tempus.pt_section, tempus.pt_network, tempus.pt_route, tempus.pt_trip, tempus.pt_stop_time, tempus.pt_frequency
  WHERE pt_network.id = pt_section.network_id AND (pt_section.stop_from = pt_stop.id OR pt_section.stop_to = pt_stop.id)
  AND pt_route.id = pt_trip.route_id AND (pt_trip.id = pt_stop_time.trip_id AND pt_stop_time.stop_id = pt_stop.id);

-- delete stops not involved in a section and not parent of another stop
delete from tempus.pt_stop
where id IN
(

select stop.id
from
   tempus.pt_stop as stop
left join
   tempus.pt_stop as stop2
on stop.id = stop2.parent_station
left join
   tempus.pt_section as s1
on s1.stop_from = stop.id
left join
   tempus.pt_section as s2
on s2.stop_to = stop.id
where
   s1.stop_from is null and
   s2.stop_to is null and
   stop2.parent_station is null
)
;


