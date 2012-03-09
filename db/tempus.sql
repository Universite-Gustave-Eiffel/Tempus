-- Tempus Database schema: version 0.8
--
-- Copyright <Olivier Courtin - 2012>
-- Licence MIT.
--
-- Version 0.9 <hugo.mercier@oslandia.com> :
-- * changed 'tc' prefixes and suffixes to 'pt' (public transport)
-- * changed 'frequence' to 'frequency'
-- * changed the 'pk' prefix to 'abscissa' (curvilinear abscissa)
-- * removed the field 'transport_type' from the 'road_section' table
-- * fixed some typos
-- * fixed the name of some IDs
-- * added a fare_id external key to the fare_rules table

--
-- DROP and clean if needed
--
DROP SCHEMA tempus CASCADE;
DELETE FROM public.geometry_columns WHERE f_table_schema='tempus';


--
-- Tempus Schema
--
CREATE SCHEMA tempus;

CREATE TABLE tempus.transport_type 
(
	id  integer PRIMARY KEY,
	parent_id integer, -- Reference to tempus.transport_type(id) => bitfield value 
	name varchar(72),
	need_parking boolean NOT NULL,  -- If vehicule need to be parked
	need_station boolean NOT NULL,  -- If vehicule is shared and use a/some stations
	need_return boolean NOT NULL    -- If vehicule is shared and must be returned 
                                        --        in the same station on the way back.
);
-- TODO Add a CHECK on parent_id related to id bitfield values

INSERT INTO tempus.transport_type VALUES (1, NULL, 'Car', 't', 'f', 'f');
INSERT INTO tempus.transport_type VALUES (2, NULL, 'Pedestrial', 'f', 'f', 'f');
INSERT INTO tempus.transport_type VALUES (4, NULL, 'cycle', 't', 'f', 'f');
INSERT INTO tempus.transport_type VALUES (8, NULL, 'Bus', 'f', 'f', 'f');
INSERT INTO tempus.transport_type VALUES (16, NULL, 'Tramway', 'f', 'f', 'f');
INSERT INTO tempus.transport_type VALUES (32, NULL, 'Metro', 'f', 'f', 'f');
INSERT INTO tempus.transport_type VALUES (64, NULL, 'Train', 'f', 'f', 'f');
INSERT INTO tempus.transport_type VALUES (128, 4, 'Shared cycle', 't', 't', 'f');
INSERT INTO tempus.transport_type VALUES (256, 1, 'Shared car', 't', 't', 'f');
INSERT INTO tempus.transport_type VALUES (512, 6, 'Roller', 'f', 'f', 'f');


--
-- Roads
-- NOTA: Consider look at OSM specification:
--       <http://wiki.openstreetmap.org/wiki/Map_Features>
--	 <http://wiki.openstreetmap.org/wiki/Tagging_samples/urban>
--       and 
--       "Description des bases de donnees vectorielles routieres" DREAL NPdC

CREATE TABLE tempus.road_type
(
	id  integer PRIMARY KEY,
	name varchar(72)
);
INSERT INTO tempus.road_type VALUES (1, 'Motorway and assimilated'); -- including peripheric 
INSERT INTO tempus.road_type VALUES (2, 'Primary roads');
INSERT INTO tempus.road_type VALUES (3, 'Secondary roads');
INSERT INTO tempus.road_type VALUES (4, 'Streets');
INSERT INTO tempus.road_type VALUES (5, 'Others'); -- still usable with car
INSERT INTO tempus.road_type VALUES (6, 'Cycle way');
INSERT INTO tempus.road_type VALUES (7, 'Pedestrial only');


CREATE TABLE tempus.road_node
(
	id bigint PRIMARY KEY,
	junction boolean NOT NULL,
	bifurcation boolean NOT NULL
	-- NOTA: geometry column added NOT NULL
);


CREATE TABLE tempus.road_section
(
	id bigint PRIMARY KEY,
	road_type integer REFERENCES tempus.road_type NOT NULL,
	node_from bigint REFERENCES tempus.road_node NOT NULL,
	node_to bigint REFERENCES tempus.road_node NOT NULL,
	transport_type_ft integer NOT NULL, -- Reference to tempus.transport_type(id) => bitfield value
	transport_type_tf integer NOT NULL, -- Reference to tempus.transport_type(id) => bitfield value
	length float(48) NOT NULL, -- in meters
	car_speed_limit float(48) NOT NULL, -- in km/h
	car_average_speed float(48), -- in km/h
	bus_average_speed float(48), -- in km/h
	road_name varchar(72),
	address_left_side varchar(72),
	address_right_side varchar(72),
	lane  integer NOT NULL DEFAULT 1,
	roundabout boolean NOT NULL,
	bridge boolean NOT NULL,
	tunnel boolean NOT NULL,
	ramp boolean NOT NULL, -- or sliproads
	tollway boolean NOT NULL
	-- NOTA: geometry column added NOT NULL
);
-- TODO Add a CHECK on transport_type_* bitfields value


CREATE TABLE tempus.road_road
(
	id integer PRIMARY KEY,
	road_section bigint[] NOT NULL,
	cost float(48) NOT NULL -- -1 mean infinite cost (i.e forbidden)
);



--
-- POI
--
CREATE TABLE tempus.poi_type
(
	id integer PRIMARY KEY,
	name varchar(96)
);

-- Some default values
INSERT INTO tempus.poi_type VALUES (1, 'Car Park');
INSERT INTO tempus.poi_type VALUES (2, 'Shared Car Point');
INSERT INTO tempus.poi_type VALUES (3, 'Cycle Park');
INSERT INTO tempus.poi_type VALUES (4, 'Shared Cycle Point');
INSERT INTO tempus.poi_type VALUES (5, 'User POI');

CREATE TABLE tempus.poi
(
	id integer PRIMARY KEY,
	poi_type integer REFERENCES tempus.poi_type NOT NULL,
	name varchar(96),
	parking_transport_type integer REFERENCES tempus.transport_type,
	road_section_id bigint REFERENCES tempus.road_section NOT NULL,
        abscissa_road_section float(48) NOT NULL
	-- NOTA: geometry column added NOT NULL
);



--
-- TC
-- NOTA: Consider look at GTFS specification
--       <http://code.google.com/transit/spec/transit_feed_specification.htm>


-- Sub Network organisation
CREATE TABLE tempus.pt_network
(
	id integer PRIMARY KEY,
	name varchar (72) NOT NULL
);

-- GTFS Stops
CREATE TABLE tempus.pt_stop
(
	id integer PRIMARY KEY,
	name varchar (72) NOT NULL,
	location_type bool, -- As in GTFS: 0 mean stop, 1 mean station
	parent_station integer REFERENCES tempus.pt_stop (id),
	road_section_id bigint REFERENCES tempus.road_section,
        zone_id integer, -- relative to fare zone
        abscissa_road_section float(48)
	-- NOTA: geometry column added NOT NULL
);


-- GTFS Routes (and subgraph link)
CREATE TABLE tempus.pt_route
(
	id integer PRIMARY KEY,
	network_id integer REFERENCES tempus.pt_network,
	short_name varchar(12) NOT NULL,
	long_name varchar(72) NOT NULL,
	route_type  integer CHECK (route_type >= 0 AND route_type <= 7)
 	-- As in GTFS: 
	-- 	0 Tram, 1 Subway/Metro, 2 Rail, 3 Bus, 4 Ferry,
    	-- 	5 Cable Car, 6 Suspended Cable car, 7 Funicular

	-- NOTA: geometry column added
);


-- Additional table added to help to display/manipulate geometry sections datas.
CREATE TABLE tempus.pt_section
(
	stop_from integer REFERENCES tempus.pt_stop NOT NULL,
	stop_to integer REFERENCES tempus.pt_stop NOT NULL,
	network_id integer REFERENCES tempus.pt_network NOT NULL,
	PRIMARY KEY(stop_from, stop_to, network_id)
	-- NOTA: geometry column added
);


-- GTFS Calendar
CREATE TABLE tempus.pt_calendar
(
	id integer PRIMARY KEY,
	monday bool NOT NULL,
	tuesday bool NOT NULL,
	wednesday bool NOT NULL,
	thursday bool NOT NULL,
	friday bool NOT NULL,
	saturday bool NOT NULL,
	sunday bool NOT NULL,
	start_date date NOT NULL,
	end_date date NOT NULL
);


-- GTFS Trip
CREATE TABLE tempus.pt_trip
(
	id bigint PRIMARY KEY, 
	route_id integer REFERENCES tempus.pt_route NOT NULL,
	service_id integer REFERENCES tempus.pt_calendar NOT NULL,
	short_name varchar(12)
	-- NOTA: shape_dist_traveled (if present) is stored as M dimension into geom
);


-- GTFS Calendar Date
CREATE TABLE tempus.pt_calendar_date
(
	service_id bigint REFERENCES tempus.pt_calendar,
	calendar_date date NOT NULL,
	exception_type integer NOT NULL,-- As in GTFS: 1 service has been added,
					--             2 service has been removed
	PRIMARY KEY (service_id, calendar_date, exception_type)
);


-- GTFS Stop Time
CREATE TABLE tempus.pt_stop_time
(
	id integer PRIMARY KEY,
	trip_id bigint REFERENCES tempus.pt_trip,
	arrival_time TIME WITHOUT TIME ZONE NOT NULL,
	departure_time TIME WITHOUT TIME ZONE NOT NULL,
	stop_id integer REFERENCES tempus.pt_stop NOT NULL,
	stop_sequence  integer NOT NULL,
	stop_headsign varchar(72),
	pickup_type  integer DEFAULT 0 CHECK(pickup_type >= 0 AND pickup_type <= 3), 	
		-- As in GTFS:
			-- 0 Regularly scheduled pickup
			-- 1 No Pickup Available
			-- 2 Must phone agency prior
			-- 3 Must coordinate with the driver 

	drop_off_type  integer DEFAULT 0,	-- Same as pickup_type 	
	shape_dist_traveled float(48)
);


-- GTFS Fare Attribute
CREATE TABLE tempus.pt_fare_attribute
(
	id integer PRIMARY KEY,
	price float(48) NOT NULL,
	currency_type char(3) DEFAULT 'EUR' NOT NULL, -- ISO 4217 codes
	transfers integer NOT NULL CHECK(transfers >= -1 AND transfers <= 2), 	
			--  0: No transfer permitted
	 		--  1: one transfer allowed
			--  2: two transfers allowed
			-- -1: unlimited transfers 
	transfers_duration  int -- expressed in seconds
);


-- GTFS Frequency
CREATE TABLE tempus.pt_frequency
(
	id integer PRIMARY KEY,
	trip_id bigint REFERENCES tempus.pt_trip,
	start_time TIME WITHOUT TIME ZONE NOT NULL,
	end_time TIME WITHOUT TIME ZONE NOT NULL,
	headways_secs integer NOT NULL
);

-- GTFS Fare Rule
CREATE TABLE tempus.pt_fare_rule
(
	id bigint PRIMARY KEY,
	fare_id bigint REFERENCES tempus.pt_fare_attribute NOT NULL,
	route_id bigint REFERENCES tempus.pt_route NOT NULL,
	origin_id integer NOT NULL, -- tempus.pt_stop (zone_id)
	destination_id integer NOT NULL,  -- tempus.pt_stop (zone_id)
	contains_id integer NOT NULL  -- tempus.pt_stop (zone_id)
);
-- TODO add CHECK on origin_id, destination_id, contains_id => zone_id


-- GTFS Transfers
CREATE TABLE tempus.pt_transfer
(
	from_stop_id integer REFERENCES tempus.pt_stop (id) NOT NULL,
	to_stop_id integer REFERENCES tempus.pt_stop (id) NOT NULL,
	transfer_type integer NOT NULL CHECK(transfert_type >=0 AND transfert_type <=3),
	--    0 or (empty) - This is a recommended transfer point between two routes.
	--    1 - This is a timed transfer point between two routes. The departing vehicle is expected to wait for the arriving one, with sufficient time for a passenger to transfer between routes.
	--    2 - This transfer requires a minimum amount of time between arrival and departure to ensure a connection. The time required to transfer is specified by min_transfer_time.
	--    3 - Transfers are not possible between routes at this location.

	min_tranfer_time integer CHECK (min_tranfer_time > 0),
	PRIMARY KEY(from_stop_id, to_stop_id, transfer_type)
);



--
-- PostGIS geometry 
--
SELECT AddGeometryColumn('tempus', 'road_section', 'geom', 2154, 'LINESTRING', 3);
SELECT AddGeometryColumn('tempus', 'road_node', 'geom', 2154, 'POINT', 3);
SELECT AddGeometryColumn('tempus', 'poi', 'geom', 2154, 'POINT', 3);
SELECT AddGeometryColumn('tempus', 'pt_stop', 'geom', 2154, 'POINT', 2);
SELECT AddGeometryColumn('tempus', 'pt_route', 'geom', 2154, 'LINESTRING', 3);
SELECT AddGeometryColumn('tempus', 'pt_section', 'geom', 2154, 'LINESTRING', 3);
-- TODO Check not empty geom
-- TODO Check valid geom
-- NOTA: EPSG:2154 means Lambert 93
