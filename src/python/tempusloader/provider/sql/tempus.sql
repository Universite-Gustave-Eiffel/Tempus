-- Tempus Database schema: version 1.1
--

--
-- DROP and clean if needed
--
CREATE EXTENSION IF NOT EXISTS postgis;

DROP SCHEMA IF EXISTS tempus CASCADE;
DELETE FROM public.geometry_columns WHERE f_table_schema='tempus';


--
-- Tempus Schema
--
CREATE SCHEMA tempus;

CREATE TABLE tempus.metadata
(
   key text primary key,
   value text
);

INSERT INTO tempus.metadata VALUES ( 'srid', '%(native_srid)' );

CREATE TABLE tempus.transport_mode
(
    id serial PRIMARY KEY,
    name varchar, -- Description of the mode
    public_transport boolean NOT NULL,
    gtfs_route_type integer, -- Reference to the equivalent GTFS codification (for PT only)
    traffic_rules integer, -- Binary composition of TransportModeTrafficRule
    speed_rule integer, -- TransportModeSpeedRule
    toll_rule integer, -- Binary composition of TransportModeToolRule
    engine_type integer, -- TransportModeEngine
    need_parking boolean,
    shared_vehicle boolean,
    return_shared_vehicle boolean
);

COMMENT ON TABLE tempus.transport_mode IS 'Available transport modes';
COMMENT ON COLUMN tempus.transport_mode.name IS 'Description of the mode';
COMMENT ON COLUMN tempus.transport_mode.traffic_rules IS 'Bitfield value: defines road traffic rules followed by the mode, NULL for PT modes';
COMMENT ON COLUMN tempus.transport_mode.gtfs_route_type IS 'Reference to the equivalent GTFS code (for PT only)';
COMMENT ON COLUMN tempus.transport_mode.speed_rule IS 'Defines the road speed rule followed by the mode, NULL for PT modes';
COMMENT ON COLUMN tempus.transport_mode.toll_rule IS 'Bitfield value: gives the toll rules followed by the mode, NULL for PT modes';
COMMENT ON COLUMN tempus.transport_mode.need_parking IS 'If vehicle needs to be parked, NULL for PT modes';
COMMENT ON COLUMN tempus.transport_mode.shared_vehicle IS 'If vehicule is shared and needs to be return at a/some stations at the end of the trip, NULL for PT modes';
COMMENT ON COLUMN tempus.transport_mode.return_shared_vehicle IS 'If vehicule is shared and needs to be returned to its initial station at the end of a loop, NULL for PT modes';
-- TODO Add a CHECK on parent_id related to id bitfield values

INSERT INTO tempus.transport_mode(name, public_transport, gtfs_route_type, traffic_rules, speed_rule, toll_rule, engine_type, need_parking, shared_vehicle, return_shared_vehicle)
	VALUES ('Walking',         'f', NULL, 1,  1, NULL, NULL, 'f', 'f', 'f');
INSERT INTO tempus.transport_mode(name, public_transport, gtfs_route_type, traffic_rules, speed_rule, toll_rule, engine_type, need_parking, shared_vehicle, return_shared_vehicle)
	VALUES ('Private bicycle', 'f', NULL, 2,  2, NULL, NULL, 't', 'f', 'f');
INSERT INTO tempus.transport_mode(name, public_transport, gtfs_route_type, traffic_rules, speed_rule, toll_rule, engine_type, need_parking, shared_vehicle, return_shared_vehicle)
	VALUES ('Private car',     'f', NULL, 4,  5, 1,    1,    't', 'f', 'f');
INSERT INTO tempus.transport_mode(name, public_transport, gtfs_route_type, traffic_rules, speed_rule, toll_rule, engine_type, need_parking, shared_vehicle, return_shared_vehicle)
        VALUES ('Taxi',            'f', NULL, 8, 5, 1,    1,    'f', 'f', 'f');

CREATE TABLE tempus.road_validity_period
(
    id integer PRIMARY KEY,
    name varchar,
    monday boolean DEFAULT true,
    tuesday boolean DEFAULT true,
    wednesday boolean DEFAULT true,
    thursday boolean DEFAULT true,
    friday boolean DEFAULT true,
    saturday boolean DEFAULT true,
    sunday boolean DEFAULT true,
    bank_holiday boolean DEFAULT true,
    day_before_bank_holiday boolean DEFAULT true,
    holidays boolean DEFAULT true,
    day_before_holidays boolean DEFAULT true,
    start_date date,
    end_date date
);
COMMENT ON TABLE tempus.road_validity_period IS 'Periods during which road restrictions and speed profiles apply';
INSERT INTO tempus.road_validity_period VALUES (0, 'Always', true, true, true, true, true, true, true, true, true, true, true, NULL, NULL);

/*
CREATE TABLE tempus.seasonal_ticket
(
    id integer PRIMARY KEY, --bitfield
    name varchar NOT NULL,
    price double precision,
    people_concerned varchar
);
INSERT INTO tempus.seasonal_ticket VALUES (1, 'Shared bicycle day ticket', 1.5, 'everybody');
INSERT INTO tempus.seasonal_ticket VALUES (2, 'Shared bicycle week ticket', 5, 'everybody');
INSERT INTO tempus.seasonal_ticket VALUES (4, 'Shared bicycle year ticket', 25, 'everybody');



CREATE TABLE tempus.road_vehicle_fare_rule
(
    id integer PRIMARY KEY,
    name varchar NOT NULL,
    seasonal_ticket integer,  -- bitfield
    transport_type integer,
    price_per_km double precision,
    price_per_minute double precision,
    price_per_use double precision,
    min_minutes integer,
    max_mintes integer
    min_km integer,
    max_km integer,
    start_time time without time zone,
    end_time time without time zone
);

INSERT INTO tempus.road_vehicle_fare_rule VALUES (1, 'Shared bicycle first half an hour of use', 7, 0, 0, 0, 0, 30, NULL, NULL, NULL, NULL);
INSERT INTO tempus.road_vehicle_fare_rule VALUES (2, 'Shared bicycle second half an hour of use', 7, 0, 0, 1, 30, 60, NULL, NULL, NULL, NULL);
INSERT INTO tempus.road_vehicle_fare_rule VALUES (3, 'Shared bicycle third half an hour of use', 7, 0, 0, 2, 60, 90, NULL, NULL, NULL, NULL);
INSERT INTO tempus.road_vehicle_fare_rule VALUES (4, 'Shared bicycle fourth half an hour of use', 7, 0, 0, 2, 90, 120, NULL, NULL, NULL, NULL);


*/
--TODO: add a data model able to represent taxis and shared vehicles fare rules => be able to give marginal cost of transports for any user category (with or without subscription to transport services...)


CREATE TABLE tempus.bank_holiday
(
    calendar_date date PRIMARY KEY,
    name varchar
);
COMMENT ON TABLE tempus.bank_holiday IS 'Bank holiday list';


CREATE TABLE tempus.holidays
(
    id serial,
    name varchar,
    start_date date,
    end_date date
);
COMMENT ON TABLE tempus.holidays IS 'Holidays description';


--
-- Roads
-- NOTA: Consider look at OSM specification:
--       <http://wiki.openstreetmap.org/wiki/Map_Features>
--	 <http://wiki.openstreetmap.org/wiki/Tagging_samples/urban>
--       and
--       "Description des bases de donnees vectorielles routieres" DREAL NPdC

CREATE TABLE tempus.road_node
(
    id bigint PRIMARY KEY,
    bifurcation boolean -- total number of incident edges is > 2
    -- NOTA: geometry column added NOT NULL
);
COMMENT ON TABLE tempus.road_node IS 'Road nodes description';
COMMENT ON COLUMN tempus.road_node.bifurcation IS 'If true, total number of incident edges is > 2';


CREATE TABLE tempus.road_section
(
    id bigint PRIMARY KEY,
    vendor_id bigint,
    road_type integer,
    node_from bigint NOT NULL REFERENCES tempus.road_node ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY IMMEDIATE,
    node_to bigint NOT NULL REFERENCES tempus.road_node ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY IMMEDIATE,
    traffic_rules_ft smallint NOT NULL, -- References tempus.road_traffic_rule => bitfield value
    traffic_rules_tf smallint NOT NULL, -- References tempus.road_traffic_rule => bitfield value
    length double precision NOT NULL, -- in meters
    car_speed_limit double precision, -- in km/h
    road_name varchar,
    lane integer,
    roundabout boolean,
    bridge boolean,
    tunnel boolean,
    ramp boolean, -- or sliproads
    tollway boolean
    -- NOTA: geometry column added NOT NULL
);
COMMENT ON TABLE tempus.road_section IS 'Road sections description';
-- TODO Add a CHECK on transport_type_* bitfields value
COMMENT ON COLUMN tempus.road_section.road_type IS '1: fast links between urban areas, 2: links between 1 level links, heavy traffic with lower speeds, 3: local links with heavy traffic, 4: low traffic';
COMMENT ON COLUMN tempus.road_section.traffic_rules_ft IS 'Bitfield value giving allowed traffic rules for direction from -> to';
COMMENT ON COLUMN tempus.road_section.traffic_rules_tf IS 'Bitfield value giving allowed traffic rules for direction to -> from';
COMMENT ON COLUMN tempus.road_section.length IS 'In meters';
COMMENT ON COLUMN tempus.road_section.car_speed_limit IS 'In km/h';
COMMENT ON COLUMN tempus.road_section.ramp IS 'Or sliproad';

CREATE TABLE tempus.road_daily_profile
(
    profile_id integer NOT NULL,
    begin_time integer NOT NULL,
    speed_rule integer NOT NULL,
    end_time  integer NOt NULL,
    average_speed double precision NOT NULL, -- In km/h
    PRIMARY KEY (profile_id, speed_rule, begin_time)
);
COMMENT ON COLUMN tempus.road_daily_profile.begin_time IS 'When the period begins. Number of minutes since midnight';
COMMENT ON COLUMN tempus.road_daily_profile.end_time IS 'When the period ends. Number of minutes since midnight';
COMMENT ON COLUMN tempus.road_daily_profile.speed_rule IS 'Speed rule: car, truck, bike, etc.';
COMMENT ON COLUMN tempus.road_daily_profile.average_speed IS 'Speed value in km/h';

CREATE TABLE tempus.road_section_speed
(
    road_section_id bigint NOT NULL REFERENCES tempus.road_section ON DELETE CASCADE ON UPDATE CASCADE,
    period_id integer NOT NULL REFERENCES tempus.road_validity_period ON DELETE CASCADE ON UPDATE CASCADE,
    profile_id integer NOT NULL, -- road_daily_profile
    PRIMARY KEY (road_section_id, period_id, profile_id)
);
COMMENT ON TABLE tempus.road_section_speed IS 'Speed, vehicle types and validity period associated to road sections';
COMMENT ON COLUMN tempus.road_section_speed.period_id IS '0 if always applies';
COMMENT ON COLUMN tempus.road_section_speed.profile_id IS 'Reference to tempus.road_daily_profile';


CREATE TABLE tempus.road_restriction
(
    id bigint PRIMARY KEY,
    sections bigint[] NOT NULL
);
COMMENT ON TABLE tempus.road_restriction IS 'Road sections lists submitted to a restriction';
COMMENT ON COLUMN tempus.road_restriction.sections IS 'Involved road sections ID, not always forming a path';

CREATE TABLE tempus.road_restriction_time_penalty
(
    restriction_id bigint NOT NULL REFERENCES tempus.road_restriction ON DELETE CASCADE ON UPDATE CASCADE,
    period_id integer REFERENCES tempus.road_validity_period ON DELETE CASCADE ON UPDATE CASCADE, -- 0 if always applies
    traffic_rules integer NOT NULL, -- References tempus.road_traffic_rule => bitfield value
    time_value double precision NOT NULL,
    PRIMARY KEY (restriction_id, period_id, traffic_rules)
);
COMMENT ON TABLE tempus.road_restriction_time_penalty IS 'Time penalty (including infinite values for forbidden movements) applied to road restrictions';
COMMENT ON COLUMN tempus.road_restriction_time_penalty.period_id IS 'NULL if always applies';
COMMENT ON COLUMN tempus.road_restriction_time_penalty.traffic_rules IS 'References tempus.transport_mode_traffic_rule => Bitfield value';
COMMENT ON COLUMN tempus.road_restriction_time_penalty.time_value IS 'In minutes';


CREATE TABLE tempus.road_restriction_toll
(
    restriction_id bigint NOT NULL REFERENCES tempus.road_restriction ON DELETE CASCADE ON UPDATE CASCADE,
    period_id integer NOT NULL REFERENCES tempus.road_validity_period ON DELETE CASCADE ON UPDATE CASCADE, -- NULL if always applies
    toll_rules integer NOT NULL, -- References tempus.road_toll_rule => bitfield value
    toll_value double precision,
    PRIMARY KEY (restriction_id, period_id, toll_rules)
);
COMMENT ON TABLE tempus.road_restriction_toll IS 'Tolls applied to road restrictions';
COMMENT ON COLUMN tempus.road_restriction_toll.period_id IS 'NULL if always applies';
COMMENT ON COLUMN tempus.road_restriction_toll.toll_rules IS 'References tempus.transport_mode_toll_rule => Bitfield value, defines the type of vehicles to which it applies';
COMMENT ON COLUMN tempus.road_restriction_toll.toll_value IS 'In euros, can be NULL if unknown';

--
-- POI
--

CREATE TABLE tempus.poi
(
	id integer PRIMARY KEY,
	poi_type integer,
	name varchar,
        parking_transport_modes integer[] NOT NULL,
	road_section_id bigint REFERENCES tempus.road_section ON DELETE CASCADE ON UPDATE CASCADE,
	abscissa_road_section double precision NOT NULL CHECK (abscissa_road_section >= 0 AND abscissa_road_section <= 1)
	-- NOTA: geometry column added NOT NULL
);
COMMENT ON TABLE tempus.poi IS 'Points of Interest description';




--
-- TC
-- NOTA: Consider look at GTFS specification
--       <http://code.google.com/transit/spec/transit_feed_specification.htm>


-- Sub Network organisation
CREATE TABLE tempus.pt_network
(
	id serial PRIMARY KEY,
	pnname varchar,
	commercial_name varchar,
	import_date timestamp not null default current_timestamp,
	calendar_begin date,
	calendar_end date
);
COMMENT ON TABLE tempus.pt_network IS 'Public transport network : one for each import operation and mostly one for each public authority';
COMMENT ON COLUMN tempus.pt_network.import_date IS 'Time and date it was imported in the database';
COMMENT ON COLUMN tempus.pt_network.calendar_begin IS 'First day of data available in the calendar of the network';
COMMENT ON COLUMN tempus.pt_network.calendar_end IS 'Last day of data available in the calendar of the network';


CREATE TABLE tempus.pt_agency
(
	id serial PRIMARY KEY,
	paname varchar NOT NULL,
	network_id integer NOT NULL REFERENCES tempus.pt_network ON DELETE CASCADE ON UPDATE CASCADE
);
COMMENT ON TABLE tempus.pt_agency
  IS 'Public transport agency : can be several agencies for one network';

CREATE TABLE tempus.pt_zone
(
    id serial PRIMARY KEY,
    vendor_id varchar,
    network_id integer NOT NULL REFERENCES tempus.pt_network ON DELETE CASCADE ON UPDATE CASCADE,
    zname varchar
);
COMMENT ON TABLE tempus.pt_zone
  IS  'Public transport fare zone: fare rule is associated to one network, if multimodal fare rule exists, a new network corresponding to a multimodal composite one needs to be defined in pt_network table';

-- GTFS Stops
CREATE TABLE tempus.pt_stop
(
	id serial PRIMARY KEY,
    vendor_id varchar, -- optional ID given by data provider
	name varchar NOT NULL,
	location_type boolean, -- As in GTFS: false means stop, true means station
	parent_station integer REFERENCES tempus.pt_stop ON DELETE CASCADE ON UPDATE CASCADE,
	road_section_id bigint REFERENCES tempus.road_section ON DELETE CASCADE ON UPDATE CASCADE,
	zone_id integer, -- relative to fare zone
	abscissa_road_section double precision -- curve length from start of road_section to the stop point
	-- NOTA: geometry column added NOT NULL
);
COMMENT ON TABLE tempus.pt_stop
  IS 'Public transport stops description';
COMMENT ON COLUMN tempus.pt_stop.vendor_id IS 'Optional ID given by the data provider';
COMMENT ON COLUMN tempus.pt_stop.location_type IS 'False means "stop", 1 means "station"';
COMMENT ON COLUMN tempus.pt_stop.zone_id IS 'Relative to fare zone';
COMMENT ON COLUMN tempus.pt_stop.abscissa_road_section IS 'Curve length from start of road_section to the stop point';


-- Routes (and subgraph link)
CREATE TABLE tempus.pt_route
(
	id serial PRIMARY KEY,
	vendor_id varchar,
	agency_id integer NOT NULL REFERENCES tempus.pt_agency ON DELETE CASCADE ON UPDATE CASCADE,
	short_name varchar,
	long_name varchar NOT NULL,
	transport_mode integer REFERENCES tempus.transport_mode

	-- NOTA: geometry column added
);
COMMENT ON TABLE tempus.pt_route
  IS 'Public transport routes description: only one route described for a line with two directions';

-- Additional table added to help to display/manipulate geometry sections datas.
CREATE TABLE tempus.pt_section
(
	stop_from integer NOT NULL REFERENCES tempus.pt_stop ON DELETE CASCADE ON UPDATE CASCADE,
	stop_to integer NOT NULL REFERENCES tempus.pt_stop ON DELETE CASCADE ON UPDATE CASCADE,
	network_id integer NOT NULL REFERENCES tempus.pt_network ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY(stop_from, stop_to, network_id)
	-- NOTA: geometry column added
);
COMMENT ON TABLE tempus.pt_section
  IS 'Public transport sections (between two subsequent stops) description';

-- Common table for sharing id between pt_calendar and pt_calendar_date
CREATE TABLE tempus.pt_service
(
        service_id bigint,
        vendor_id varchar,
        PRIMARY KEY (service_id)
);

-- GTFS Calendar
CREATE TABLE tempus.pt_calendar
(
	service_id bigint NOT NULL REFERENCES tempus.pt_service ON DELETE CASCADE ON UPDATE CASCADE,
	monday boolean NOT NULL,
	tuesday boolean NOT NULL,
	wednesday boolean NOT NULL,
	thursday boolean NOT NULL,
	friday boolean NOT NULL,
	saturday boolean NOT NULL,
	sunday boolean NOT NULL,
	start_date date NOT NULL,
	end_date date NOT NULL,
        PRIMARY KEY (service_id)
);
COMMENT ON TABLE tempus.pt_calendar
  IS 'Public transport regular services description';

-- GTFS Calendar Date
CREATE TABLE tempus.pt_calendar_date
(
	service_id bigint NOT NULL REFERENCES tempus.pt_service ON DELETE CASCADE ON UPDATE CASCADE,
	calendar_date date NOT NULL,
	exception_type integer NOT NULL,-- As in GTFS: 1 service has been added,
					--             2 service has been removed
	PRIMARY KEY (service_id, calendar_date, exception_type)
);
COMMENT ON TABLE tempus.pt_calendar_date
  IS 'Public transport exceptional services description';
COMMENT ON COLUMN tempus.pt_calendar_date.exception_type IS 'As in GTFS : 1 service has been added, 2 service has been removed';

-- GTFS Trip
CREATE TABLE tempus.pt_trip
(
	id serial PRIMARY KEY,
	vendor_id varchar,
	route_id integer NOT NULL REFERENCES tempus.pt_route ON DELETE CASCADE ON UPDATE CASCADE,
        -- no foreign key here, because service_id may refer either pt_calendar OR pt_calendar_date
        -- FIXME
	service_id integer NOT NULL,
	short_name varchar
	-- NOTA: shape_dist_traveled (if present) is stored as M dimension into geom
);
COMMENT ON TABLE tempus.pt_trip
  IS 'Public transport trips description';

-- GTFS Stop Time
CREATE TABLE tempus.pt_stop_time
(
	id serial PRIMARY KEY,
	trip_id bigint NOT NULL REFERENCES tempus.pt_trip ON DELETE CASCADE ON UPDATE CASCADE,
	arrival_time TIME WITHOUT TIME ZONE NOT NULL,
	departure_time TIME WITHOUT TIME ZONE NOT NULL,
	stop_id integer NOT NULL REFERENCES tempus.pt_stop ON DELETE CASCADE ON UPDATE CASCADE,
	stop_sequence  integer NOT NULL,
	stop_headsign varchar,
	pickup_type  integer DEFAULT 0 CHECK(pickup_type >= 0 AND pickup_type <= 3),
		-- As in GTFS:
			-- 0 Regularly scheduled pickup
			-- 1 No Pickup Available
			-- 2 Must phone agency prior
			-- 3 Must coordinate with the driver

	drop_off_type  integer DEFAULT 0,	-- Same as pickup_type
	shape_dist_traveled double precision
);
COMMENT ON TABLE tempus.pt_stop_time
  IS 'Public transport service timetables';
COMMENT ON COLUMN tempus.pt_stop_time.pickup_type IS 'As in GTFS: 0 Regularly scheduled pickup, 1 No Pickup Available, 2 Must phone agency prior, 3 Must coordinate with the driver';
COMMENT ON COLUMN tempus.pt_stop_time.pickup_type IS 'Same as pickup_type';


-- GTFS Fare Attribute
CREATE TABLE tempus.pt_fare_attribute
(
	id serial PRIMARY KEY,
	vendor_id varchar,
	price double precision NOT NULL,
	currency_type char(3) DEFAULT 'EUR' NOT NULL, -- ISO 4217 codes
	transfers integer NOT NULL CHECK(transfers >= -1 AND transfers <= 2),
			--  0: No transfer allowed
	 		--  1: one transfer allowed
			--  2: two transfers allowed
			-- -1: unlimited transfers
	transfers_duration  integer -- expressed in seconds
);
COMMENT ON TABLE tempus.pt_fare_attribute
  IS 'Public transport fare types description';
COMMENT ON COLUMN tempus.pt_fare_attribute.currency_type IS 'ISO 4217 code';
COMMENT ON COLUMN tempus.pt_fare_attribute.transfers IS 'As in GTFS: 0 No transfer allowed, 1 One transfer allowed, 2 Two transfers allowed, -1 Unlimited transfers';
COMMENT ON COLUMN tempus.pt_fare_attribute.transfers_duration IS 'Duration in which transfers are allowed (in seconds)';


-- GTFS Frequency
-- DROP TABLE tempus.pt_frequency;
CREATE TABLE tempus.pt_frequency
(
	id serial PRIMARY KEY,
	trip_id bigint REFERENCES tempus.pt_trip(id),
	start_time TIME WITHOUT TIME ZONE NOT NULL,
	end_time TIME WITHOUT TIME ZONE NOT NULL,
	headway_secs integer NOT NULL
);
COMMENT ON TABLE tempus.pt_frequency
  IS 'Public transport service frequencies';

-- GTFS Fare Rule
-- DROP TABLE tempus.pt_fare_rule;
CREATE TABLE tempus.pt_fare_rule
(
	id serial PRIMARY KEY,
	fare_id bigint REFERENCES tempus.pt_fare_attribute NOT NULL,
	route_id bigint REFERENCES tempus.pt_route NOT NULL,
	origin_id integer NOT NULL, -- tempus.pt_stop (zone_id)
	destination_id integer NOT NULL,  -- tempus.pt_stop (zone_id)
	contains_id integer NOT NULL  -- tempus.pt_stop (zone_id)
);
-- TODO add CHECK on origin_id, destination_id, contains_id => zone_id
COMMENT ON TABLE tempus.pt_fare_rule
  IS 'Public transport fare system description';
COMMENT ON COLUMN tempus.pt_fare_rule.origin_id IS 'Zone ID referenced in tempus.pt_stop';
COMMENT ON COLUMN tempus.pt_fare_rule.destination_id IS 'Zone ID referenced in tempus.pt_stop';
COMMENT ON COLUMN tempus.pt_fare_rule.contains_id IS 'Zone ID referenced in tempus.pt_stop';

-- TO DO: CASCADE DELETE from pt_network to pt_fare_rule and pt_fare_attribute

--
-- PostGIS geometry
--
SELECT AddGeometryColumn('tempus', 'road_section', 'geom', %(native_srid), 'LINESTRING', 3);
SELECT AddGeometryColumn('tempus', 'road_node', 'geom', %(native_srid), 'POINT', 3);
SELECT AddGeometryColumn('tempus', 'poi', 'geom', %(native_srid), 'POINT', 3);
SELECT AddGeometryColumn('tempus', 'pt_stop', 'geom', %(native_srid), 'POINT', 3);
SELECT AddGeometryColumn('tempus', 'pt_route', 'geom', %(native_srid), 'LINESTRING', 3);
SELECT AddGeometryColumn('tempus', 'pt_section', 'geom', %(native_srid), 'LINESTRING', 3);
-- TODO Check not empty geom
-- TODO Check valid geom

--
-- Utilitary functions
--


-- find the closest road_section, then select the closest endpoint
create or replace function tempus.road_node_id_from_coordinates( float8, float8 ) returns bigint
as '
with rs as (
select id, node_from, node_to from tempus.road_section
order by geom <-> st_setsrid(st_point($1, $2), %(native_srid)) limit 1
)
select case when st_distance( p1.geom, st_setsrid(st_point($1,$2), %(native_srid))) < st_distance( p2.geom, st_setsrid(st_point($1,$2), %(native_srid))) then p1.id else p2.id end
from rs, tempus.road_node as p1, tempus.road_node as p2
where
rs.node_from = p1.id and rs.node_to = p2.id
'
language sql;

create or replace function tempus.road_node_id_from_coordinates_and_modes( float8, float8, int[] = array[1] ) returns bigint
as '
with rs as (
select road_section.id, node_from, node_to from tempus.road_section, tempus.transport_mode
where
  transport_mode.id in (select unnest($3)) and
  (transport_mode.traffic_rules & traffic_rules_ft = transport_mode.traffic_rules
   or
  transport_mode.traffic_rules & traffic_rules_tf = transport_mode.traffic_rules)
order by geom <-> st_setsrid(st_point($1, $2), %(native_srid)) limit 1
)
select case when st_distance( p1.geom, st_setsrid(st_point($1,$2), %(native_srid))) < st_distance( p2.geom, st_setsrid(st_point($1,$2), %(native_srid))) then p1.id else p2.id end
from rs, tempus.road_node as p1, tempus.road_node as p2
where
rs.node_from = p1.id and rs.node_to = p2.id
'
language sql;

DROP VIEW IF EXISTS tempus.view_forbidden_movements;
CREATE OR REPLACE VIEW tempus.view_forbidden_movements AS
 SELECT road_restriction.id,
    road_restriction.sections,
    st_union(road_section.geom) AS geom,
    max(road_restriction_time_penalty.traffic_rules) AS traffic_rules
   FROM tempus.road_section,
    tempus.road_restriction,
    tempus.road_restriction_time_penalty
  WHERE (road_section.id = ANY (road_restriction.sections)) AND road_restriction_time_penalty.restriction_id = road_restriction.id AND road_restriction_time_penalty.time_value = 'Infinity'::double precision
  GROUP BY road_restriction.id, road_restriction.sections;

DROP FUNCTION IF EXISTS tempus.array_search(anyelement, anyarray);

CREATE OR REPLACE FUNCTION tempus.array_search(needle anyelement, haystack anyarray)
  RETURNS integer AS
$BODY$
    SELECT i
      FROM generate_subscripts($2, 1) AS i
     WHERE $2[i] = $1
  ORDER BY i
$BODY$
  LANGUAGE sql STABLE
  COST 100;
ALTER FUNCTION tempus.array_search(anyelement, anyarray)
  OWNER TO postgres;

-- service_id <-> network_id mapping
create materialized view tempus.pt_service_network as
select
  distinct service_id, network_id
from
  tempus.pt_network
join
  tempus.pt_agency
on pt_agency.network_id = pt_network.id
join
  tempus.pt_route
on pt_route.agency_id = pt_agency.id
join
  tempus.pt_trip
on pt_trip.route_id = pt_route.id
;

-- days of service for each service
create materialized view tempus.pt_service_day as
with network_days as
   (select
      n.id as network_id,
      calendar_begin+dt as sday
    from
      tempus.pt_network as n join lateral
      generate_series(1, n.calendar_end - n.calendar_begin) as dt on true
    )
 select
   service_id,
   network_id,
   sday
 from
   tempus.pt_calendar,
   tempus.pt_network,
   network_days
 where
   (ARRAY[sunday, monday, tuesday, wednesday, thursday, friday, saturday])[EXTRACT(dow FROM sday)+1]
 union
 select
   cd.service_id,
   sn.network_id,
   calendar_date
 from
   tempus.pt_calendar_date cd,
   network_days,
   tempus.pt_service_network sn
 where
   exception_type = 1
   and cd.service_id = sn.service_id 
 except
 select
   cd.service_id,
   sn.network_id,
   calendar_date
 from
   tempus.pt_calendar_date cd,
   network_days,
   tempus.pt_service_network sn
 where
   exception_type = 2
   and cd.service_id = sn.service_id 
;

-- for each pair of pt stops, departure, arrival_time and service_id of each available trip
create materialized view tempus.pt_timetable as
select
  network_id,
  t1.stop_id as origin_stop,
  t2.stop_id as destination_stop,
  pt_route.transport_mode,
  t1.trip_id,
  extract(epoch from t1.arrival_time)/60 as departure_time,
  extract(epoch from t2.departure_time)/60 as arrival_time,
  service_id
from
  tempus.pt_stop_time t1,
  tempus.pt_stop_time t2,
  tempus.pt_trip,
  tempus.pt_route,
  tempus.pt_agency
where
  t1.trip_id = t2.trip_id
  and t1.stop_sequence + 1 = t2.stop_sequence
  and pt_trip.id = t1.trip_id
  and pt_route.id = pt_trip.route_id
  and pt_agency.id = pt_route.agency_id
;

-- Preparing pt views

CREATE OR REPLACE FUNCTION tempus.update_pt_views() RETURNS void as $$
BEGIN
-- pseudo materialized views
drop table if exists tempus.view_stop_by_network;
create table tempus.view_stop_by_network as
 SELECT DISTINCT ON (pt_stop.id, pt_route.transport_mode, pt_network.id) int4(row_number() OVER ()) AS id,
    pt_stop.id AS stop_id,
    pt_stop.vendor_id,
    pt_stop.name,
    pt_stop.location_type,
    pt_stop.parent_station,
    transport_mode.name AS transport_mode_name,
    transport_mode.gtfs_route_type,
    pt_stop.road_section_id,
    pt_stop.zone_id,
    pt_stop.abscissa_road_section,
    pt_stop.geom,
    pt_network.id AS network_id
   FROM tempus.pt_stop,
    tempus.pt_section,
    tempus.pt_network,
    tempus.pt_route,
    tempus.pt_trip,
    tempus.pt_stop_time,
    tempus.transport_mode
  WHERE pt_network.id = pt_section.network_id AND (pt_section.stop_from = pt_stop.id OR pt_section.stop_to = pt_stop.id) AND pt_route.id = pt_trip.route_id AND pt_trip.id = pt_stop_time.trip_id AND pt_stop_time.stop_id = pt_stop.id AND transport_mode.id = pt_route.transport_mode;

DROP TABLE IF EXISTS tempus.view_section_by_network;
CREATE TABLE tempus.view_section_by_network AS
 SELECT DISTINCT ON (pt_section.stop_from, pt_section.stop_to, pt_section.network_id, transport_mode.name) int4(row_number() OVER ()) AS id,
    pt_section.stop_from,
    pt_section.stop_to,
    pt_section.network_id,
    transport_mode.name AS transport_mode_name,
    transport_mode.gtfs_route_type,
    pt_section.geom
   FROM tempus.pt_section,
    tempus.pt_route,
    tempus.pt_trip,
    tempus.pt_stop_time pt_stop_time_from,
    tempus.pt_stop_time pt_stop_time_to,
    tempus.transport_mode
  WHERE pt_route.id = pt_trip.route_id AND pt_trip.id = pt_stop_time_from.trip_id AND pt_stop_time_from.trip_id = pt_stop_time_to.trip_id AND pt_stop_time_from.stop_sequence = (pt_stop_time_to.stop_sequence - 1) AND pt_stop_time_from.stop_id = pt_section.stop_from AND pt_stop_time_to.stop_id = pt_section.stop_to AND transport_mode.id = pt_route.transport_mode;
  END;
$$ LANGUAGE plpgsql;

refresh materialized view tempus.pt_service_day;

refresh materialized view tempus.pt_timetable;

select tempus.update_pt_views();

-- subsets metadata

CREATE TABLE tempus.subset
(
	id serial PRIMARY KEY,
        schema_name text NOT NULL
);
-- polygon that has been used to define the subset
SELECT AddGeometryColumn('tempus', 'subset', 'geom', %(native_srid), 'POLYGON', 2);

CREATE OR REPLACE FUNCTION tempus.create_subset(msubset text, polygon text) RETURNS void AS $$
DECLARE
    mdefinition text;
    -- declarations
BEGIN
  EXECUTE format('DROP SCHEMA IF EXISTS %s CASCADE', msubset);
  EXECUTE format('delete from geometry_columns where f_table_schema=''%s''', msubset);
  EXECUTE format('create schema %s', msubset);

  -- road nodes
  EXECUTE format('create table %s.road_node as select rn.* from tempus.road_node as rn where st_intersects( ''' || polygon || '''::geometry, rn.geom )', msubset);
  -- road sections
  EXECUTE format( 'create table %s.road_section as ' ||
	'select rs.* from tempus.road_section as rs ' ||
	'where ' ||
        'node_from in (select id from %1$s.road_node) ' ||
        'and node_to in (select id from %1$s.road_node)', msubset);
  -- pt stops
  EXECUTE format( 'create table %s.pt_stop as ' ||
                   'select pt.* from tempus.pt_stop as pt, %1$s.road_section as rs where road_section_id = rs.id', msubset );

  -- pt section
  EXECUTE format( 'create table %s.pt_section as ' ||
		'select pt.* from tempus.pt_section as pt ' ||
	        'where stop_from in (select id from %1$s.pt_stop) ' ||
                'and stop_to in (select id from %1$s.pt_stop)', msubset );

  -- pt stop time
  EXECUTE format( 'create table %s.pt_stop_time as ' ||
	'select st.* from tempus.pt_stop_time as st, %1$s.pt_stop as stop where stop_id = stop.id', msubset );

  -- poi
  EXECUTE format( 'create table %s.poi as ' ||
                  'select poi.* from tempus.poi as poi, %1$s.road_section as rs where road_section_id = rs.id', msubset );

  -- road_restriction
  EXECUTE format( 'create table %s.road_restriction as ' ||
         'select distinct rr.id, rr.sections from tempus.road_restriction as rr, %1$s.road_section as rs ' ||
         'where rs.id in (select unnest(sections) from tempus.road_restriction where id=rr.id)', msubset );

  EXECUTE format( 'create table %s.road_restriction_time_penalty as ' ||
         'select rrtp.* from tempus.road_restriction_time_penalty as rrtp, ' ||
         '%1$s.road_restriction as rr where restriction_id = rr.id', msubset );

  -- pt_trip
  EXECUTE format( 'create table %s.pt_trip as ' ||
                  'select * from tempus.pt_trip where id in (select distinct trip_id from %1$s.pt_stop_time union select trip_id from tempus.pt_frequency)', msubset);
  -- pt_frequency
  EXECUTE format( 'create table %s.pt_frequency as select * from tempus.pt_frequency', msubset );

  -- pt_route
  EXECUTE format( 'create table %s.pt_route as select * from tempus.pt_route where id in (select distinct route_id from %1$s.pt_trip)', msubset );

  -- pt_calendar
  EXECUTE format( 'create table %s.pt_calendar as select * from tempus.pt_calendar where service_id in (select distinct service_id from %1$s.pt_trip)', msubset );
  EXECUTE format( 'create table %s.pt_calendar_date as select * from tempus.pt_calendar_date where service_id in (select distinct service_id from %1$s.pt_trip)', msubset );

  -- pt_network
  EXECUTE format( 'create table %s.pt_network as select * from tempus.pt_network where id in (select distinct network_id from %1$s.pt_section)', msubset );
  -- pt_agency
  EXECUTE format( 'create table %s.pt_agency as select * from tempus.pt_agency where network_id in (select distinct id from %1$s.pt_network)', msubset );
  -- pt_zone
  EXECUTE format( 'create table %s.pt_zone as select * from tempus.pt_zone where network_id in (select distinct id from %1$s.pt_network)', msubset );
  -- pt_fare_rule
  EXECUTE format( 'create table %s.pt_fare_rule as select * from tempus.pt_fare_rule where route_id in (select id from %1$s.pt_route)', msubset );
  -- pt_fare_attribute
  EXECUTE format( 'create table %s.pt_fare_attribute as select * from tempus.pt_fare_attribute', msubset );

  -- transport_mode
  EXECUTE format( 'create table %s.transport_mode as select * from tempus.transport_mode', msubset );

  -- forbidden movements view
  SELECT replace(definition, 'tempus.', msubset || '.') into mdefinition from pg_views where schemaname='tempus' and viewname='view_forbidden_movements';
  EXECUTE 'create view ' || msubset || '.view_forbidden_movements as ' || mdefinition;

  -- update_pt_views
  select replace(prosrc, 'tempus.', msubset || '.') into mdefinition
     from pg_proc, pg_namespace where pg_proc.pronamespace = pg_namespace.oid and proname='update_pt_views' and nspname='tempus';
  EXECUTE 'create function ' || msubset || '.update_pt_views() returns void as $' || '$' || mdefinition || '$' || '$ language plpgsql';
  EXECUTE 'SELECT ' || msubset || '.update_pt_views()';

  DELETE FROM tempus.subset WHERE schema_name=msubset;
  INSERT INTO tempus.subset (schema_name, geom) VALUES (msubset, polygon::geometry);
END;
$$ LANGUAGE plpgsql;
