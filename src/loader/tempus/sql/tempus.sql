-- Tempus Database schema: version 1.1
--

--
-- DROP and clean if needed
--

DROP SCHEMA IF EXISTS tempus CASCADE;
DELETE FROM public.geometry_columns WHERE f_table_schema='tempus';


--
-- Tempus Schema
--
CREATE SCHEMA tempus;

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
	VALUES ('Taxi',            'f', NULL, 12, 5, 1,    1,    'f', 'f', 'f'); 

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
    road_type integer NOT NULL,
    node_from bigint NOT NULL REFERENCES tempus.road_node ON DELETE CASCADE ON UPDATE CASCADE,
    node_to bigint NOT NULL REFERENCES tempus.road_node ON DELETE CASCADE ON UPDATE CASCADE,
    traffic_rules_ft integer NOT NULL, -- References tempus.road_traffic_rule => bitfield value
    traffic_rules_tf integer NOT NULL, -- References tempus.road_traffic_rule => bitfield value
    length double precision NOT NULL, -- in meters
    car_speed_limit double precision, -- in km/h
    road_name varchar,
    lane integer,
    roundabout boolean NOT NULL,
    bridge boolean NOT NULL,
    tunnel boolean NOT NULL,
    ramp boolean NOT NULL, -- or sliproads
    tollway boolean NOT NULL 
    -- NOTA: geometry column added NOT NULL
);
COMMENT ON TABLE tempus.road_section IS 'Road sections description';
-- TODO Add a CHECK on transport_type_* bitfields value
COMMENT ON COLUMN tempus.road_section.traffic_rules_ft IS 'Bitfield value giving allowed traffic rules for direction from -> to'; 
COMMENT ON COLUMN tempus.road_section.traffic_rules_tf IS 'Bitfield value giving allowed traffic rules for direction to -> from'; 
COMMENT ON COLUMN tempus.road_section.length IS 'In meters'; 
COMMENT ON COLUMN tempus.road_section.car_speed_limit IS 'In km/h'; 
COMMENT ON COLUMN tempus.road_section.ramp IS 'Or sliproad'; 


CREATE TABLE tempus.road_section_speed
(
    road_section_id bigint NOT NULL REFERENCES tempus.road_section ON DELETE CASCADE ON UPDATE CASCADE, 
    period_id integer NOT NULL REFERENCES tempus.road_validity_period ON DELETE CASCADE ON UPDATE CASCADE, 
    speed_rule integer NOT NULL, 
    speed_value double precision NOT NULL, -- In km/h
    PRIMARY KEY (road_section_id, period_id, speed_rule)
); 
COMMENT ON TABLE tempus.road_section_speed IS 'Speed, vehicle types and validity period associated to road sections';
COMMENT ON COLUMN tempus.road_section_speed.period_id IS '0 if always applies'; 
COMMENT ON COLUMN tempus.road_section_speed.speed_value IS 'Speed value in km/h'; 


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
COMMENT ON COLUMN tempus.road_restriction_toll.toll_value IS 'In €, can be NULL if unknown'; 

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

-- GTFS Calendar
CREATE TABLE tempus.pt_calendar
(
	service_id bigint,
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
	service_id bigint,
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

-- GTFS Transfers
CREATE TABLE tempus.pt_transfer
(
	from_stop_id integer REFERENCES tempus.pt_stop (id) ON DELETE CASCADE ON UPDATE CASCADE,
	to_stop_id integer REFERENCES tempus.pt_stop (id) ON DELETE CASCADE ON UPDATE CASCADE,
	transfer_type integer NOT NULL CHECK(transfer_type >=0 AND transfer_type <=3),
	--    0 or (empty) - This is a recommended transfer point between two routes.
	--    1 - This is a timed transfer point between two routes. The departing vehicle is expected to wait for the arriving one, with sufficient time for a passenger to transfer between routes.
	--    2 - This transfer requires a minimum amount of time between arrival and departure to ensure a connection. The time required to transfer is specified by min_transfer_time.
	--    3 - Transfers are not possible between routes at this location.
	min_tranfer_time integer CHECK (min_tranfer_time > 0),
	PRIMARY KEY(from_stop_id, to_stop_id, transfer_type)
);
COMMENT ON TABLE tempus.pt_transfer
  IS 'Public transport transfer permission and necessary time';
COMMENT ON COLUMN tempus.pt_transfer.transfer_type IS 'As in GTFS: 0 recommanded transfer point, 1 timed transfer point : the departing vehicle is expected to wait the arriving one, 2 transfer which requires a minimum amount of time between arrival and departure to ensure connection, specified in min_transfer_time, 3 impossible to make transfers at this point'; 


--
-- PostGIS geometry 
--
SELECT AddGeometryColumn('tempus', 'road_section', 'geom', 2154, 'LINESTRING', 3);
SELECT AddGeometryColumn('tempus', 'road_node', 'geom', 2154, 'POINT', 3);
SELECT AddGeometryColumn('tempus', 'poi', 'geom', 2154, 'POINT', 3);
SELECT AddGeometryColumn('tempus', 'pt_stop', 'geom', 2154, 'POINT', 3);
SELECT AddGeometryColumn('tempus', 'pt_route', 'geom', 2154, 'LINESTRING', 3);
SELECT AddGeometryColumn('tempus', 'pt_section', 'geom', 2154, 'LINESTRING', 3);
-- TODO Check not empty geom
-- TODO Check valid geom
-- NOTA: EPSG:2154 means Lambert 93

--
-- Utilitary functions
--


-- find the closest road_section, then select the closest endpoint
create or replace function tempus.road_node_id_from_coordinates( float8, float8 ) returns bigint
as '
with rs as (
select id, node_from, node_to from tempus.road_section
order by geom <-> st_setsrid(st_point($1, $2), 2154) limit 1
)
select case when st_distance( p1.geom, st_setsrid(st_point($1,$2), 2154)) < st_distance( p2.geom, st_setsrid(st_point($1,$2), 2154)) then p1.id else p2.id end
from rs, tempus.road_node as p1, tempus.road_node as p2
where
rs.node_from = p1.id and rs.node_to = p2.id
'
language sql;

DROP VIEW IF EXISTS tempus.forbidden_movements;
CREATE OR REPLACE VIEW tempus.forbidden_movements AS
SELECT
	road_restriction.id,
	road_restriction.sections,
	st_union(road_section.geom) AS geom
FROM tempus.road_section, tempus.road_restriction
WHERE road_section.id = ANY (road_restriction.sections)
GROUP BY road_restriction.id;

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

CREATE OR REPLACE FUNCTION tempus.create_subset(subset text, polygon text) RETURNS void AS $$
--DECLARE
    -- declarations
BEGIN
  EXECUTE format('DROP SCHEMA IF EXISTS %s CASCADE', subset);
  EXECUTE format('delete from geometry_columns where f_table_schema=''%s''', subset);
  EXECUTE format('create schema %s', subset);

  -- road nodes
  EXECUTE format('create table %s.road_node as select rn.* from tempus.road_node as rn where st_intersects( ''' || polygon || '''::geometry, rn.geom )', subset);
  -- road sections
  EXECUTE format( 'create table %s.road_section as ' ||
	'select rs.* from tempus.road_section as rs ' ||
	'where ' ||
        'node_from in (select id from %1$s.road_node) ' ||
        'and node_to in (select id from %1$s.road_node)', subset);
  -- pt stops
  EXECUTE format( 'create table %s.pt_stop as ' ||
                   'select pt.* from tempus.pt_stop as pt, %1$s.road_section as rs where road_section_id = rs.id', subset );

  -- pt section
  EXECUTE format( 'create table %s.pt_section as ' ||
		'select pt.* from tempus.pt_section as pt ' ||
	        'where stop_from in (select id from %1$s.pt_stop) ' ||
                'and stop_to in (select id from %1$s.pt_stop)', subset );

  -- pt stop time
  EXECUTE format( 'create table %s.pt_stop_time as ' ||
	'select st.* from tempus.pt_stop_time as st, %1$s.pt_stop as stop where stop_id = stop.id', subset );

  -- poi
  EXECUTE format( 'create table %s.poi as ' ||
                  'select poi.* from tempus.poi as poi, %1$s.road_section as rs where road_section_id = rs.id', subset );

  -- road_restriction
  EXECUTE format( 'create table %s.road_restriction as ' ||
         'select distinct rr.id, rr.sections from tempus.road_restriction as rr, %1$s.road_section as rs ' ||
         'where rs.id in (select unnest(sections) from tempus.road_restriction where id=rr.id)', subset );

  EXECUTE format( 'create table %s.road_restriction_time_penalty as ' ||
         'select rrtp.* from tempus.road_restriction_time_penalty as rrtp, ' ||
         '%1$s.road_restriction as rr where restriction_id = rr.id', subset );

END;
$$ LANGUAGE plpgsql;

