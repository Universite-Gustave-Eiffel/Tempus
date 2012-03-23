/* Create GTFS tables in the tempus import schema for raw import of GTFS data */

/* Create reference tables */
DROP TABLE IF EXISTS _tempus_import._tempus_import.route_types; 
CREATE TABLE route_types (
    route_type integer,
    description text
);

INSERT INTO route_types VALUES (0,'streetcar/light rail');
INSERT INTO route_types VALUES (1,'subway/metro');
INSERT INTO route_types VALUES (2,'rail');
INSERT INTO route_types VALUES (3,'bus');
INSERT INTO route_types VALUES (4,'ferry');
INSERT INTO route_types VALUES (5,'cable car');
INSERT INTO route_types VALUES (6,'gondola');
INSERT INTO route_types VALUES (7,'funicular');

/* GTFS data tables */
DROP TABLE IF EXISTS _tempus_import.agency;
CREATE TABLE agency (
    agency_id character varying,
    agency_name character varying NOT NULL,
    agency_url character varying NOT NULL,
    agency_timezone character varying NOT NULL,
    agency_lang character varying,
    agency_phone character varying,
    agency_fare_url character varying
);

DROP TABLE IF EXISTS _tempus_import.calendar;
CREATE TABLE calendar (
    service_id character varying NOT NULL,
    monday integer NOT NULL,
    tuesday integer NOT NULL,
    wednesday integer NOT NULL,
    thursday integer NOT NULL,
    friday integer NOT NULL,
    saturday integer NOT NULL,
    sunday integer NOT NULL,
    start_date character varying NOT NULL,
    end_date character varying NOT NULL
);

DROP TABLE IF EXISTS _tempus_import.calendar_dates;
CREATE TABLE calendar_dates (
    service_id character varying NOT NULL,
    "date" character varying NOT NULL,
    exception_type character varying NOT NULL
);


DROP TABLE IF EXISTS _tempus_import.fare_attributes; 
CREATE TABLE fare_attributes (
    fare_id character varying NOT NULL,
    price character varying NOT NULL,
    currency_type character varying NOT NULL,
    payment_method integer NOT NULL,
    transfers integer,
    transfer_duration integer
);

DROP TABLE IF EXISTS _tempus_import.fare_rules;
CREATE TABLE fare_rules (
    fare_id character varying NOT NULL,
    route_id character varying,
    origin_id character varying,
    destination_id character varying,
    contains_id character varying
);

DROP TABLE IF EXISTS _tempus_import.frequencies;
CREATE TABLE frequencies (
    trip_id character varying NOT NULL,
    start_time character varying NOT NULL,
    end_time character varying NOT NULL,
    headway_secs integer NOT NULL
);


DROP TABLE IF EXISTS _tempus_import.routes; 
CREATE TABLE routes (
    agency_id character varying,
    route_id character varying NOT NULL,
    route_short_name character varying,
    route_long_name character varying NOT NULL,
    route_desc character varying,
    route_type integer NOT NULL,
    route_url character varying,
    route_color character varying,
    route_text_color character varying
);

DROP TABLE IF EXISTS _tempus_import.shapes;
CREATE TABLE shapes (
    shape_id character varying NOT NULL,
    shape_pt_lat double precision NOT NULL,
    shape_pt_lon double precision NOT NULL,
    shape_pt_sequence integer NOT NULL,
    shape_dist_traveled double precision,
    the_geom geometry
);

DROP TABLE IF EXISTS _tempus_import.stop_times;
CREATE TABLE stop_times (
    trip_id character varying NOT NULL,
    arrival_time character varying NOT NULL,
    departure_time character varying NOT NULL,
    stop_id character varying NOT NULL,
    stop_sequence integer NOT NULL,
    stop_headsign character varying,
    pickup_type integer,
    drop_off_type integer,
    shape_dist_traveled integer
);

DROP TABLE IF EXISTS _tempus_import.stops;
CREATE TABLE stops (
    stop_id character varying NOT NULL,
    stop_code character varying,
    stop_name character varying NOT NULL,
    stop_desc character varying,
    stop_lat double precision NOT NULL,
    stop_lon double precision NOT NULL,
    zone_id character varying,
    stop_url character varying,
    location_type integer,
    parent_station character varying,
    the_geom geometry
);


DROP TABLE IF EXISTS _tempus_import.trips;
CREATE TABLE trips (
    route_id character varying NOT NULL,
    service_id character varying NOT NULL,
    trip_id character varying NOT NULL,
    trip_headsign character varying,
    trip_short_name character varying,
    direction_id integer,
    block_id character varying,
    shape_id character varying,
    direction varchar
);

