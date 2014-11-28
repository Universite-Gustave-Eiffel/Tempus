--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: tempus; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA tempus;


SET search_path = tempus, pg_catalog;

--
-- Name: array_search(anyelement, anyarray); Type: FUNCTION; Schema: tempus; Owner: -
--

CREATE FUNCTION array_search(needle anyelement, haystack anyarray) RETURNS integer
    LANGUAGE sql STABLE
    AS $_$
    SELECT i
      FROM generate_subscripts($2, 1) AS i
     WHERE $2[i] = $1
  ORDER BY i
$_$;


--
-- Name: create_subset(text, text); Type: FUNCTION; Schema: tempus; Owner: -
--

CREATE FUNCTION create_subset(msubset text, polygon text) RETURNS void
    LANGUAGE plpgsql
    AS $_$
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
$_$;


--
-- Name: road_node_id_from_coordinates(double precision, double precision); Type: FUNCTION; Schema: tempus; Owner: -
--

CREATE FUNCTION road_node_id_from_coordinates(double precision, double precision) RETURNS bigint
    LANGUAGE sql
    AS $_$
with rs as (
select id, node_from, node_to from tempus.road_section
order by geom <-> st_setsrid(st_point($1, $2), 2154) limit 1
)
select case when st_distance( p1.geom, st_setsrid(st_point($1,$2), 2154)) < st_distance( p2.geom, st_setsrid(st_point($1,$2), 2154)) then p1.id else p2.id end
from rs, tempus.road_node as p1, tempus.road_node as p2
where
rs.node_from = p1.id and rs.node_to = p2.id
$_$;


--
-- Name: road_node_id_from_coordinates_and_modes(double precision, double precision, integer[]); Type: FUNCTION; Schema: tempus; Owner: -
--

CREATE FUNCTION road_node_id_from_coordinates_and_modes(double precision, double precision, integer[] DEFAULT ARRAY[1]) RETURNS bigint
    LANGUAGE sql
    AS $_$
with rs as (
select road_section.id, node_from, node_to from tempus.road_section, tempus.transport_mode
where
  transport_mode.id in (select unnest($3)) and
  (transport_mode.traffic_rules & traffic_rules_ft = transport_mode.traffic_rules
   or
  transport_mode.traffic_rules & traffic_rules_tf = transport_mode.traffic_rules)
order by geom <-> st_setsrid(st_point($1, $2), 2154) limit 1
)
select case when st_distance( p1.geom, st_setsrid(st_point($1,$2), 2154)) < st_distance( p2.geom, st_setsrid(st_point($1,$2), 2154)) then p1.id else p2.id end
from rs, tempus.road_node as p1, tempus.road_node as p2
where
rs.node_from = p1.id and rs.node_to = p2.id
$_$;


--
-- Name: update_pt_views(); Type: FUNCTION; Schema: tempus; Owner: -
--

CREATE FUNCTION update_pt_views() RETURNS void
    LANGUAGE plpgsql
    AS $$
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
$$;


SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: bank_holiday; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE bank_holiday (
    calendar_date date NOT NULL,
    name character varying
);


--
-- Name: TABLE bank_holiday; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE bank_holiday IS 'Bank holiday list';


--
-- Name: holidays; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE holidays (
    id integer NOT NULL,
    name character varying,
    start_date date,
    end_date date
);


--
-- Name: TABLE holidays; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE holidays IS 'Holidays description';


--
-- Name: holidays_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE holidays_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: holidays_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE holidays_id_seq OWNED BY holidays.id;


--
-- Name: poi; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE poi (
    id integer NOT NULL,
    poi_type integer,
    name character varying,
    parking_transport_modes integer[] NOT NULL,
    road_section_id bigint,
    abscissa_road_section double precision NOT NULL,
    geom public.geometry(PointZ,2154),
    CONSTRAINT poi_abscissa_road_section_check CHECK (((abscissa_road_section >= (0)::double precision) AND (abscissa_road_section <= (1)::double precision)))
);


--
-- Name: TABLE poi; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE poi IS 'Points of Interest description';


--
-- Name: pt_agency; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_agency (
    id integer NOT NULL,
    paname character varying NOT NULL,
    network_id integer NOT NULL
);


--
-- Name: TABLE pt_agency; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_agency IS 'Public transport agency : can be several agencies for one network';


--
-- Name: pt_agency_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_agency_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_agency_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_agency_id_seq OWNED BY pt_agency.id;


--
-- Name: pt_calendar; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_calendar (
    service_id bigint NOT NULL,
    monday boolean NOT NULL,
    tuesday boolean NOT NULL,
    wednesday boolean NOT NULL,
    thursday boolean NOT NULL,
    friday boolean NOT NULL,
    saturday boolean NOT NULL,
    sunday boolean NOT NULL,
    start_date date NOT NULL,
    end_date date NOT NULL
);


--
-- Name: TABLE pt_calendar; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_calendar IS 'Public transport regular services description';


--
-- Name: pt_calendar_date; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_calendar_date (
    service_id bigint NOT NULL,
    calendar_date date NOT NULL,
    exception_type integer NOT NULL
);


--
-- Name: TABLE pt_calendar_date; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_calendar_date IS 'Public transport exceptional services description';


--
-- Name: COLUMN pt_calendar_date.exception_type; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_calendar_date.exception_type IS 'As in GTFS : 1 service has been added, 2 service has been removed';


--
-- Name: pt_fare_attribute; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_fare_attribute (
    id integer NOT NULL,
    vendor_id character varying,
    price double precision NOT NULL,
    currency_type character(3) DEFAULT 'EUR'::bpchar NOT NULL,
    transfers integer NOT NULL,
    transfers_duration integer,
    CONSTRAINT pt_fare_attribute_transfers_check CHECK (((transfers >= (-1)) AND (transfers <= 2)))
);


--
-- Name: TABLE pt_fare_attribute; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_fare_attribute IS 'Public transport fare types description';


--
-- Name: COLUMN pt_fare_attribute.currency_type; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_fare_attribute.currency_type IS 'ISO 4217 code';


--
-- Name: COLUMN pt_fare_attribute.transfers; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_fare_attribute.transfers IS 'As in GTFS: 0 No transfer allowed, 1 One transfer allowed, 2 Two transfers allowed, -1 Unlimited transfers';


--
-- Name: COLUMN pt_fare_attribute.transfers_duration; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_fare_attribute.transfers_duration IS 'Duration in which transfers are allowed (in seconds)';


--
-- Name: pt_fare_attribute_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_fare_attribute_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_fare_attribute_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_fare_attribute_id_seq OWNED BY pt_fare_attribute.id;


--
-- Name: pt_fare_rule; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_fare_rule (
    id integer NOT NULL,
    fare_id bigint NOT NULL,
    route_id bigint NOT NULL,
    origin_id integer NOT NULL,
    destination_id integer NOT NULL,
    contains_id integer NOT NULL
);


--
-- Name: TABLE pt_fare_rule; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_fare_rule IS 'Public transport fare system description';


--
-- Name: COLUMN pt_fare_rule.origin_id; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_fare_rule.origin_id IS 'Zone ID referenced in tempus.pt_stop';


--
-- Name: COLUMN pt_fare_rule.destination_id; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_fare_rule.destination_id IS 'Zone ID referenced in tempus.pt_stop';


--
-- Name: COLUMN pt_fare_rule.contains_id; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_fare_rule.contains_id IS 'Zone ID referenced in tempus.pt_stop';


--
-- Name: pt_fare_rule_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_fare_rule_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_fare_rule_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_fare_rule_id_seq OWNED BY pt_fare_rule.id;


--
-- Name: pt_frequency; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_frequency (
    id integer NOT NULL,
    trip_id bigint,
    start_time time without time zone NOT NULL,
    end_time time without time zone NOT NULL,
    headway_secs integer NOT NULL
);


--
-- Name: TABLE pt_frequency; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_frequency IS 'Public transport service frequencies';


--
-- Name: pt_frequency_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_frequency_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_frequency_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_frequency_id_seq OWNED BY pt_frequency.id;


--
-- Name: pt_network; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_network (
    id integer NOT NULL,
    pnname character varying,
    commercial_name character varying,
    import_date timestamp without time zone DEFAULT now() NOT NULL,
    calendar_begin date,
    calendar_end date
);


--
-- Name: TABLE pt_network; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_network IS 'Public transport network : one for each import operation and mostly one for each public authority';


--
-- Name: COLUMN pt_network.import_date; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_network.import_date IS 'Time and date it was imported in the database';


--
-- Name: COLUMN pt_network.calendar_begin; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_network.calendar_begin IS 'First day of data available in the calendar of the network';


--
-- Name: COLUMN pt_network.calendar_end; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_network.calendar_end IS 'Last day of data available in the calendar of the network';


--
-- Name: pt_network_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_network_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_network_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_network_id_seq OWNED BY pt_network.id;


--
-- Name: pt_route; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_route (
    id integer NOT NULL,
    vendor_id character varying,
    agency_id integer NOT NULL,
    short_name character varying,
    long_name character varying NOT NULL,
    transport_mode integer,
    geom public.geometry(LineStringZ,2154)
);


--
-- Name: TABLE pt_route; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_route IS 'Public transport routes description: only one route described for a line with two directions';


--
-- Name: pt_route_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_route_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_route_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_route_id_seq OWNED BY pt_route.id;


--
-- Name: pt_section; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_section (
    stop_from integer NOT NULL,
    stop_to integer NOT NULL,
    network_id integer NOT NULL,
    geom public.geometry(LineStringZ,2154)
);


--
-- Name: TABLE pt_section; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_section IS 'Public transport sections (between two subsequent stops) description';


--
-- Name: pt_service; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_service (
    service_id bigint NOT NULL,
    vendor_id character varying
);


--
-- Name: pt_stop; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_stop (
    id integer NOT NULL,
    vendor_id character varying,
    name character varying NOT NULL,
    location_type boolean,
    parent_station integer,
    road_section_id bigint,
    zone_id integer,
    abscissa_road_section double precision,
    geom public.geometry(PointZ,2154)
);


--
-- Name: TABLE pt_stop; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_stop IS 'Public transport stops description';


--
-- Name: COLUMN pt_stop.vendor_id; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_stop.vendor_id IS 'Optional ID given by the data provider';


--
-- Name: COLUMN pt_stop.location_type; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_stop.location_type IS 'False means "stop", 1 means "station"';


--
-- Name: COLUMN pt_stop.zone_id; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_stop.zone_id IS 'Relative to fare zone';


--
-- Name: COLUMN pt_stop.abscissa_road_section; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_stop.abscissa_road_section IS 'Curve length from start of road_section to the stop point';


--
-- Name: pt_stop_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_stop_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_stop_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_stop_id_seq OWNED BY pt_stop.id;


--
-- Name: pt_stop_time; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_stop_time (
    id integer NOT NULL,
    trip_id bigint NOT NULL,
    arrival_time time without time zone NOT NULL,
    departure_time time without time zone NOT NULL,
    stop_id integer NOT NULL,
    stop_sequence integer NOT NULL,
    stop_headsign character varying,
    pickup_type integer DEFAULT 0,
    drop_off_type integer DEFAULT 0,
    shape_dist_traveled double precision,
    CONSTRAINT pt_stop_time_pickup_type_check CHECK (((pickup_type >= 0) AND (pickup_type <= 3)))
);


--
-- Name: TABLE pt_stop_time; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_stop_time IS 'Public transport service timetables';


--
-- Name: COLUMN pt_stop_time.pickup_type; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN pt_stop_time.pickup_type IS 'Same as pickup_type';


--
-- Name: pt_stop_time_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_stop_time_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_stop_time_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_stop_time_id_seq OWNED BY pt_stop_time.id;


--
-- Name: pt_trip; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_trip (
    id integer NOT NULL,
    vendor_id character varying,
    route_id integer NOT NULL,
    service_id integer NOT NULL,
    short_name character varying
);


--
-- Name: TABLE pt_trip; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_trip IS 'Public transport trips description';


--
-- Name: pt_trip_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_trip_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_trip_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_trip_id_seq OWNED BY pt_trip.id;


--
-- Name: pt_zone; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE pt_zone (
    id integer NOT NULL,
    vendor_id character varying,
    network_id integer NOT NULL,
    zname character varying
);


--
-- Name: TABLE pt_zone; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE pt_zone IS 'Public transport fare zone: fare rule is associated to one network, if multimodal fare rule exists, a new network corresponding to a multimodal composite one needs to be defined in pt_network table';


--
-- Name: pt_zone_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE pt_zone_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: pt_zone_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE pt_zone_id_seq OWNED BY pt_zone.id;


--
-- Name: road_daily_profile; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE road_daily_profile (
    profile_id integer NOT NULL,
    begin_time integer NOT NULL,
    speed_rule integer NOT NULL,
    end_time integer NOT NULL,
    average_speed double precision NOT NULL
);


--
-- Name: road_node; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE road_node (
    id bigint NOT NULL,
    bifurcation boolean,
    geom public.geometry(PointZ,2154)
);


--
-- Name: TABLE road_node; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE road_node IS 'Road nodes description';


--
-- Name: COLUMN road_node.bifurcation; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_node.bifurcation IS 'If true, total number of incident edges is > 2';


--
-- Name: road_restriction; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE road_restriction (
    id bigint NOT NULL,
    sections bigint[] NOT NULL
);


--
-- Name: TABLE road_restriction; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE road_restriction IS 'Road sections lists submitted to a restriction';


--
-- Name: COLUMN road_restriction.sections; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_restriction.sections IS 'Involved road sections ID, not always forming a path';


--
-- Name: road_restriction_time_penalty; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE road_restriction_time_penalty (
    restriction_id bigint NOT NULL,
    period_id integer NOT NULL,
    traffic_rules integer NOT NULL,
    time_value double precision NOT NULL
);


--
-- Name: TABLE road_restriction_time_penalty; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE road_restriction_time_penalty IS 'Time penalty (including infinite values for forbidden movements) applied to road restrictions';


--
-- Name: COLUMN road_restriction_time_penalty.period_id; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_restriction_time_penalty.period_id IS 'NULL if always applies';


--
-- Name: COLUMN road_restriction_time_penalty.traffic_rules; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_restriction_time_penalty.traffic_rules IS 'References tempus.transport_mode_traffic_rule => Bitfield value';


--
-- Name: COLUMN road_restriction_time_penalty.time_value; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_restriction_time_penalty.time_value IS 'In minutes';


--
-- Name: road_restriction_toll; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE road_restriction_toll (
    restriction_id bigint NOT NULL,
    period_id integer NOT NULL,
    toll_rules integer NOT NULL,
    toll_value double precision
);


--
-- Name: TABLE road_restriction_toll; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE road_restriction_toll IS 'Tolls applied to road restrictions';


--
-- Name: COLUMN road_restriction_toll.period_id; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_restriction_toll.period_id IS 'NULL if always applies';


--
-- Name: COLUMN road_restriction_toll.toll_rules; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_restriction_toll.toll_rules IS 'References tempus.transport_mode_toll_rule => Bitfield value, defines the type of vehicles to which it applies';


--
-- Name: COLUMN road_restriction_toll.toll_value; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_restriction_toll.toll_value IS 'In €, can be NULL if unknown';


--
-- Name: road_section; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE road_section (
    id bigint NOT NULL,
    road_type integer NOT NULL,
    node_from bigint NOT NULL,
    node_to bigint NOT NULL,
    traffic_rules_ft integer NOT NULL,
    traffic_rules_tf integer NOT NULL,
    length double precision NOT NULL,
    car_speed_limit double precision,
    road_name character varying,
    lane integer,
    roundabout boolean NOT NULL,
    bridge boolean NOT NULL,
    tunnel boolean NOT NULL,
    ramp boolean NOT NULL,
    tollway boolean NOT NULL,
    geom public.geometry(LineStringZ,2154)
);


--
-- Name: TABLE road_section; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE road_section IS 'Road sections description';


--
-- Name: COLUMN road_section.road_type; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_section.road_type IS '1: fast links between urban areas, 2: links between 1 level links, heavy traffic with lower speeds, 3: local links with heavy traffic, 4: low traffic';


--
-- Name: COLUMN road_section.traffic_rules_ft; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_section.traffic_rules_ft IS 'Bitfield value giving allowed traffic rules for direction from -> to';


--
-- Name: COLUMN road_section.traffic_rules_tf; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_section.traffic_rules_tf IS 'Bitfield value giving allowed traffic rules for direction to -> from';


--
-- Name: COLUMN road_section.length; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_section.length IS 'In meters';


--
-- Name: COLUMN road_section.car_speed_limit; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_section.car_speed_limit IS 'In km/h';


--
-- Name: COLUMN road_section.ramp; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN road_section.ramp IS 'Or sliproad';


--
-- Name: road_section_speed; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE road_section_speed (
    road_section_id bigint NOT NULL,
    period_id integer NOT NULL,
    profile_id integer NOT NULL
);


--
-- Name: road_validity_period; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE road_validity_period (
    id integer NOT NULL,
    name character varying,
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


--
-- Name: TABLE road_validity_period; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE road_validity_period IS 'Periods during which road restrictions and speed profiles apply';


--
-- Name: subset; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE subset (
    id integer NOT NULL,
    schema_name text NOT NULL,
    geom public.geometry(Polygon,2154)
);


--
-- Name: subset_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE subset_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: subset_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE subset_id_seq OWNED BY subset.id;


--
-- Name: transport_mode; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE transport_mode (
    id integer NOT NULL,
    name character varying,
    public_transport boolean NOT NULL,
    gtfs_route_type integer,
    traffic_rules integer,
    speed_rule integer,
    toll_rule integer,
    engine_type integer,
    need_parking boolean,
    shared_vehicle boolean,
    return_shared_vehicle boolean
);


--
-- Name: TABLE transport_mode; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON TABLE transport_mode IS 'Available transport modes';


--
-- Name: COLUMN transport_mode.name; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN transport_mode.name IS 'Description of the mode';


--
-- Name: COLUMN transport_mode.gtfs_route_type; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN transport_mode.gtfs_route_type IS 'Reference to the equivalent GTFS code (for PT only)';


--
-- Name: COLUMN transport_mode.traffic_rules; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN transport_mode.traffic_rules IS 'Bitfield value: defines road traffic rules followed by the mode, NULL for PT modes';


--
-- Name: COLUMN transport_mode.speed_rule; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN transport_mode.speed_rule IS 'Defines the road speed rule followed by the mode, NULL for PT modes';


--
-- Name: COLUMN transport_mode.toll_rule; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN transport_mode.toll_rule IS 'Bitfield value: gives the toll rules followed by the mode, NULL for PT modes';


--
-- Name: COLUMN transport_mode.need_parking; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN transport_mode.need_parking IS 'If vehicle needs to be parked, NULL for PT modes';


--
-- Name: COLUMN transport_mode.shared_vehicle; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN transport_mode.shared_vehicle IS 'If vehicule is shared and needs to be return at a/some stations at the end of the trip, NULL for PT modes';


--
-- Name: COLUMN transport_mode.return_shared_vehicle; Type: COMMENT; Schema: tempus; Owner: -
--

COMMENT ON COLUMN transport_mode.return_shared_vehicle IS 'If vehicule is shared and needs to be returned to its initial station at the end of a loop, NULL for PT modes';


--
-- Name: transport_mode_id_seq; Type: SEQUENCE; Schema: tempus; Owner: -
--

CREATE SEQUENCE transport_mode_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: transport_mode_id_seq; Type: SEQUENCE OWNED BY; Schema: tempus; Owner: -
--

ALTER SEQUENCE transport_mode_id_seq OWNED BY transport_mode.id;


--
-- Name: view_forbidden_movements; Type: VIEW; Schema: tempus; Owner: -
--

CREATE VIEW view_forbidden_movements AS
 SELECT road_restriction.id,
    road_restriction.sections,
    public.st_union(road_section.geom) AS geom,
    max(road_restriction_time_penalty.traffic_rules) AS traffic_rules
   FROM road_section,
    road_restriction,
    road_restriction_time_penalty
  WHERE (((road_section.id = ANY (road_restriction.sections)) AND (road_restriction_time_penalty.restriction_id = road_restriction.id)) AND (road_restriction_time_penalty.time_value = 'Infinity'::double precision))
  GROUP BY road_restriction.id, road_restriction.sections;


--
-- Name: view_section_by_network; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE view_section_by_network (
    id integer,
    stop_from integer,
    stop_to integer,
    network_id integer,
    transport_mode_name character varying,
    gtfs_route_type integer,
    geom public.geometry(LineStringZ,2154)
);


--
-- Name: view_stop_by_network; Type: TABLE; Schema: tempus; Owner: -; Tablespace: 
--

CREATE TABLE view_stop_by_network (
    id integer,
    stop_id integer,
    vendor_id character varying,
    name character varying,
    location_type boolean,
    parent_station integer,
    transport_mode_name character varying,
    gtfs_route_type integer,
    road_section_id bigint,
    zone_id integer,
    abscissa_road_section double precision,
    geom public.geometry(PointZ,2154),
    network_id integer
);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY holidays ALTER COLUMN id SET DEFAULT nextval('holidays_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_agency ALTER COLUMN id SET DEFAULT nextval('pt_agency_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_fare_attribute ALTER COLUMN id SET DEFAULT nextval('pt_fare_attribute_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_fare_rule ALTER COLUMN id SET DEFAULT nextval('pt_fare_rule_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_frequency ALTER COLUMN id SET DEFAULT nextval('pt_frequency_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_network ALTER COLUMN id SET DEFAULT nextval('pt_network_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_route ALTER COLUMN id SET DEFAULT nextval('pt_route_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_stop ALTER COLUMN id SET DEFAULT nextval('pt_stop_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_stop_time ALTER COLUMN id SET DEFAULT nextval('pt_stop_time_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_trip ALTER COLUMN id SET DEFAULT nextval('pt_trip_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_zone ALTER COLUMN id SET DEFAULT nextval('pt_zone_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY subset ALTER COLUMN id SET DEFAULT nextval('subset_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY transport_mode ALTER COLUMN id SET DEFAULT nextval('transport_mode_id_seq'::regclass);


--
-- Data for Name: bank_holiday; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY bank_holiday (calendar_date, name) FROM stdin;
\.


--
-- Data for Name: holidays; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY holidays (id, name, start_date, end_date) FROM stdin;
\.


--
-- Name: holidays_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('holidays_id_seq', 1, false);


--
-- Data for Name: poi; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY poi (id, poi_type, name, parking_transport_modes, road_section_id, abscissa_road_section, geom) FROM stdin;
\.


--
-- Data for Name: pt_agency; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_agency (id, paname, network_id) FROM stdin;
1	Transport	1
\.


--
-- Name: pt_agency_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_agency_id_seq', 1, false);


--
-- Data for Name: pt_calendar; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_calendar (service_id, monday, tuesday, wednesday, thursday, friday, saturday, sunday, start_date, end_date) FROM stdin;
1	t	t	t	t	t	t	t	2000-01-01	2031-12-31
\.


--
-- Data for Name: pt_calendar_date; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_calendar_date (service_id, calendar_date, exception_type) FROM stdin;
\.


--
-- Data for Name: pt_fare_attribute; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_fare_attribute (id, vendor_id, price, currency_type, transfers, transfers_duration) FROM stdin;
\.


--
-- Name: pt_fare_attribute_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_fare_attribute_id_seq', 1, false);


--
-- Data for Name: pt_fare_rule; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_fare_rule (id, fare_id, route_id, origin_id, destination_id, contains_id) FROM stdin;
\.


--
-- Name: pt_fare_rule_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_fare_rule_id_seq', 1, false);


--
-- Data for Name: pt_frequency; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_frequency (id, trip_id, start_time, end_time, headway_secs) FROM stdin;
\.


--
-- Name: pt_frequency_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_frequency_id_seq', 1, false);


--
-- Data for Name: pt_network; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_network (id, pnname, commercial_name, import_date, calendar_begin, calendar_end) FROM stdin;
1	Transport network		2014-11-13 00:00:00	0200-01-01	2031-12-31
\.


--
-- Name: pt_network_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_network_id_seq', 1, false);


--
-- Data for Name: pt_route; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_route (id, vendor_id, agency_id, short_name, long_name, transport_mode, geom) FROM stdin;
1		1	Route A	Route A	5	\N
2		1	Route B	Route B	5	\N
\.


--
-- Name: pt_route_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_route_id_seq', 1, false);


--
-- Data for Name: pt_section; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_section (stop_from, stop_to, network_id, geom) FROM stdin;
3	2	1	01020000A06A08000002000000C56ECA40819915416A60C93FF58359410000000000000000FE83A435F99F15417FFDA837F18359410000000000000000
2	1	1	01020000A06A08000002000000FE83A435F99F15417FFDA837F18359410000000000000000ABC41BDC3CA715414697A8B4E88359410000000000000000
5	4	1	01020000A06A08000002000000FEC8D7D62C7815419221930BE68359410000000000000000875DC87C568415415DB78775ED8359410000000000000000
\.


--
-- Data for Name: pt_service; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_service (service_id, vendor_id) FROM stdin;
1	1
\.


--
-- Data for Name: pt_stop; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_stop (id, vendor_id, name, location_type, parent_station, road_section_id, zone_id, abscissa_road_section, geom) FROM stdin;
1	\N	A	\N	\N	1	\N	1	01010000A06A080000ABC41BDC3CA715414697A8B4E88359410000000000000000
2	\N	B	\N	\N	2	\N	1	01010000A06A080000FE83A435F99F15417FFDA837F18359410000000000000000
5	\N	TTB	\N	\N	6	\N	1	01010000A06A080000FEC8D7D62C7815419221930BE68359410000000000000000
3	\N	C	\N	\N	3	\N	1	01010000A06A080000C56ECA40819915416A60C93FF58359410000000000000000
4	\N	TTA	\N	\N	5	\N	1	01010000A06A080000875DC87C568415415DB78775ED8359410000000000000000
\.


--
-- Name: pt_stop_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_stop_id_seq', 1, false);


--
-- Data for Name: pt_stop_time; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) FROM stdin;
1	1	16:41:00	16:41:00	3	1		0	0	0
2	1	16:43:00	16:43:00	2	2		0	0	0
3	1	16:45:00	16:45:00	1	3		0	0	0
4	2	16:30:00	16:30:00	5	1		0	0	0
5	2	16:32:00	16:32:00	4	2		0	0	0
6	3	16:31:00	16:31:00	3	1		0	0	0
7	3	16:33:00	16:33:00	2	2		0	0	0
8	3	16:35:00	16:35:00	1	3		0	0	0
10	4	16:22:00	16:22:00	4	2		0	0	0
9	4	16:20:00	16:20:00	5	1		0	0	0
\.


--
-- Name: pt_stop_time_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_stop_time_id_seq', 1, false);


--
-- Data for Name: pt_trip; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_trip (id, vendor_id, route_id, service_id, short_name) FROM stdin;
1		1	1	A to C
2		2	1	TTA to TTB
3		1	1	C to A2
4		2	1	TTA to TTB2
\.


--
-- Name: pt_trip_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_trip_id_seq', 1, false);


--
-- Data for Name: pt_zone; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY pt_zone (id, vendor_id, network_id, zname) FROM stdin;
\.


--
-- Name: pt_zone_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('pt_zone_id_seq', 1, false);


--
-- Data for Name: road_daily_profile; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY road_daily_profile (profile_id, begin_time, speed_rule, end_time, average_speed) FROM stdin;
1	0	5	1440	30
2	0	5	480	90
2	480	5	600	70
2	600	5	720	90
2	720	5	840	50
2	840	5	990	90
2	990	5	1200	70
2	1200	5	1440	90
\.


--
-- Data for Name: road_node; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY road_node (id, bifurcation, geom) FROM stdin;
1	\N	01010000A06A0800006B48729105B115418E61EC03398459410000000000000000
2	\N	01010000A06A080000EB3F48628EA6154194DE4C5C388459410000000000000000
3	\N	01010000A06A080000C1E647E3BF9F154105E8AE61358459410000000000000000
4	\N	01010000A06A08000038EB03BE199915417EC19D7F308459410000000000000000
5	\N	01010000A06A0800007844F745D48F15415734F2AA2F8459410000000000000000
6	\N	01010000A06A080000F142FF4464841541B9C8291E318459410000000000000000
7	\N	01010000A06A08000050B4F0B8A1781541FEEB95B8318459410000000000000000
8	\N	01010000A06A080000C8B8CFBAEE6C15418B2423D9378459410000000000000000
\.


--
-- Data for Name: road_restriction; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY road_restriction (id, sections) FROM stdin;
\.


--
-- Data for Name: road_restriction_time_penalty; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY road_restriction_time_penalty (restriction_id, period_id, traffic_rules, time_value) FROM stdin;
\.


--
-- Data for Name: road_restriction_toll; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY road_restriction_toll (restriction_id, period_id, toll_rules, toll_value) FROM stdin;
\.


--
-- Data for Name: road_section; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY road_section (id, road_type, node_from, node_to, traffic_rules_ft, traffic_rules_tf, length, car_speed_limit, road_name, lane, roundabout, bridge, tunnel, ramp, tollway, geom) FROM stdin;
1	5	1	2	255	255	600	90	E1	\N	f	f	f	f	f	01020000A06A080000020000006B48729105B115418E61EC03398459410000000000000000EB3F48628EA6154194DE4C5C388459410000000000000000
2	5	2	3	255	255	10000	90	E2	\N	f	f	f	f	f	01020000A06A08000002000000EB3F48628EA6154194DE4C5C388459410000000000000000C1E647E3BF9F154105E8AE61358459410000000000000000
3	5	3	4	255	255	10000	90	E3	\N	f	f	f	f	f	01020000A06A08000002000000C1E647E3BF9F154105E8AE6135845941000000000000000038EB03BE199915417EC19D7F308459410000000000000000
4	5	4	5	255	255	180	90	E4	\N	f	f	f	f	f	01020000A06A0800000200000038EB03BE199915417EC19D7F3084594100000000000000007844F745D48F15415734F2AA2F8459410000000000000000
5	5	5	6	255	255	180	90	E5	\N	f	f	f	f	f	01020000A06A080000020000007844F745D48F15415734F2AA2F8459410000000000000000F142FF4464841541B9C8291E318459410000000000000000
6	5	6	7	255	255	10000	90	E6	\N	f	f	f	f	f	01020000A06A08000002000000F142FF4464841541B9C8291E31845941000000000000000050B4F0B8A1781541FEEB95B8318459410000000000000000
7	5	7	8	255	255	180	90	E7	\N	f	f	f	f	f	01020000A06A0800000200000050B4F0B8A1781541FEEB95B8318459410000000000000000C8B8CFBAEE6C15418B2423D9378459410000000000000000
\.


--
-- Data for Name: road_section_speed; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY road_section_speed (road_section_id, period_id, profile_id) FROM stdin;
1	0	1
2	0	1
4	0	2
5	0	2
\.


--
-- Data for Name: road_validity_period; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY road_validity_period (id, name, monday, tuesday, wednesday, thursday, friday, saturday, sunday, bank_holiday, day_before_bank_holiday, holidays, day_before_holidays, start_date, end_date) FROM stdin;
0	Always	t	t	t	t	t	t	t	t	t	t	t	\N	\N
\.


--
-- Data for Name: subset; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY subset (id, schema_name, geom) FROM stdin;
\.


--
-- Name: subset_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('subset_id_seq', 1, false);


--
-- Data for Name: transport_mode; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY transport_mode (id, name, public_transport, gtfs_route_type, traffic_rules, speed_rule, toll_rule, engine_type, need_parking, shared_vehicle, return_shared_vehicle) FROM stdin;
1	Walking	f	\N	1	1	\N	\N	f	f	f
2	Private bicycle	f	\N	2	2	\N	\N	t	f	f
3	Private car	f	\N	4	5	1	1	t	f	f
4	Taxi	f	\N	8	5	1	1	f	f	f
5	PT	t	5	\N	\N	\N	\N	f	f	f
\.


--
-- Name: transport_mode_id_seq; Type: SEQUENCE SET; Schema: tempus; Owner: -
--

SELECT pg_catalog.setval('transport_mode_id_seq', 4, true);


--
-- Data for Name: view_section_by_network; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY view_section_by_network (id, stop_from, stop_to, network_id, transport_mode_name, gtfs_route_type, geom) FROM stdin;
\.


--
-- Data for Name: view_stop_by_network; Type: TABLE DATA; Schema: tempus; Owner: -
--

COPY view_stop_by_network (id, stop_id, vendor_id, name, location_type, parent_station, transport_mode_name, gtfs_route_type, road_section_id, zone_id, abscissa_road_section, geom, network_id) FROM stdin;
\.


--
-- Name: bank_holiday_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY bank_holiday
    ADD CONSTRAINT bank_holiday_pkey PRIMARY KEY (calendar_date);


--
-- Name: poi_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY poi
    ADD CONSTRAINT poi_pkey PRIMARY KEY (id);


--
-- Name: pt_agency_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_agency
    ADD CONSTRAINT pt_agency_pkey PRIMARY KEY (id);


--
-- Name: pt_calendar_date_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_calendar_date
    ADD CONSTRAINT pt_calendar_date_pkey PRIMARY KEY (service_id, calendar_date, exception_type);


--
-- Name: pt_calendar_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_calendar
    ADD CONSTRAINT pt_calendar_pkey PRIMARY KEY (service_id);


--
-- Name: pt_fare_attribute_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_fare_attribute
    ADD CONSTRAINT pt_fare_attribute_pkey PRIMARY KEY (id);


--
-- Name: pt_fare_rule_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_fare_rule
    ADD CONSTRAINT pt_fare_rule_pkey PRIMARY KEY (id);


--
-- Name: pt_frequency_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_frequency
    ADD CONSTRAINT pt_frequency_pkey PRIMARY KEY (id);


--
-- Name: pt_network_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_network
    ADD CONSTRAINT pt_network_pkey PRIMARY KEY (id);


--
-- Name: pt_route_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_route
    ADD CONSTRAINT pt_route_pkey PRIMARY KEY (id);


--
-- Name: pt_section_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_section
    ADD CONSTRAINT pt_section_pkey PRIMARY KEY (stop_from, stop_to, network_id);


--
-- Name: pt_service_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_service
    ADD CONSTRAINT pt_service_pkey PRIMARY KEY (service_id);


--
-- Name: pt_stop_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_stop
    ADD CONSTRAINT pt_stop_pkey PRIMARY KEY (id);


--
-- Name: pt_stop_time_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_stop_time
    ADD CONSTRAINT pt_stop_time_pkey PRIMARY KEY (id);


--
-- Name: pt_trip_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_trip
    ADD CONSTRAINT pt_trip_pkey PRIMARY KEY (id);


--
-- Name: pt_zone_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY pt_zone
    ADD CONSTRAINT pt_zone_pkey PRIMARY KEY (id);


--
-- Name: road_daily_profile_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY road_daily_profile
    ADD CONSTRAINT road_daily_profile_pkey PRIMARY KEY (profile_id, begin_time);


--
-- Name: road_node_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY road_node
    ADD CONSTRAINT road_node_pkey PRIMARY KEY (id);


--
-- Name: road_restriction_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY road_restriction
    ADD CONSTRAINT road_restriction_pkey PRIMARY KEY (id);


--
-- Name: road_restriction_time_penalty_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY road_restriction_time_penalty
    ADD CONSTRAINT road_restriction_time_penalty_pkey PRIMARY KEY (restriction_id, period_id, traffic_rules);


--
-- Name: road_restriction_toll_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY road_restriction_toll
    ADD CONSTRAINT road_restriction_toll_pkey PRIMARY KEY (restriction_id, period_id, toll_rules);


--
-- Name: road_section_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY road_section
    ADD CONSTRAINT road_section_pkey PRIMARY KEY (id);


--
-- Name: road_section_speed_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY road_section_speed
    ADD CONSTRAINT road_section_speed_pkey PRIMARY KEY (road_section_id, period_id, profile_id);


--
-- Name: road_validity_period_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY road_validity_period
    ADD CONSTRAINT road_validity_period_pkey PRIMARY KEY (id);


--
-- Name: subset_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY subset
    ADD CONSTRAINT subset_pkey PRIMARY KEY (id);


--
-- Name: transport_mode_pkey; Type: CONSTRAINT; Schema: tempus; Owner: -; Tablespace: 
--

ALTER TABLE ONLY transport_mode
    ADD CONSTRAINT transport_mode_pkey PRIMARY KEY (id);


--
-- Name: poi_road_section_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY poi
    ADD CONSTRAINT poi_road_section_id_fkey FOREIGN KEY (road_section_id) REFERENCES road_section(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_agency_network_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_agency
    ADD CONSTRAINT pt_agency_network_id_fkey FOREIGN KEY (network_id) REFERENCES pt_network(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_calendar_date_service_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_calendar_date
    ADD CONSTRAINT pt_calendar_date_service_id_fkey FOREIGN KEY (service_id) REFERENCES pt_service(service_id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_calendar_service_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_calendar
    ADD CONSTRAINT pt_calendar_service_id_fkey FOREIGN KEY (service_id) REFERENCES pt_service(service_id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_fare_rule_fare_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_fare_rule
    ADD CONSTRAINT pt_fare_rule_fare_id_fkey FOREIGN KEY (fare_id) REFERENCES pt_fare_attribute(id);


--
-- Name: pt_fare_rule_route_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_fare_rule
    ADD CONSTRAINT pt_fare_rule_route_id_fkey FOREIGN KEY (route_id) REFERENCES pt_route(id);


--
-- Name: pt_frequency_trip_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_frequency
    ADD CONSTRAINT pt_frequency_trip_id_fkey FOREIGN KEY (trip_id) REFERENCES pt_trip(id);


--
-- Name: pt_route_agency_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_route
    ADD CONSTRAINT pt_route_agency_id_fkey FOREIGN KEY (agency_id) REFERENCES pt_agency(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_route_transport_mode_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_route
    ADD CONSTRAINT pt_route_transport_mode_fkey FOREIGN KEY (transport_mode) REFERENCES transport_mode(id);


--
-- Name: pt_section_network_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_section
    ADD CONSTRAINT pt_section_network_id_fkey FOREIGN KEY (network_id) REFERENCES pt_network(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_section_stop_from_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_section
    ADD CONSTRAINT pt_section_stop_from_fkey FOREIGN KEY (stop_from) REFERENCES pt_stop(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_section_stop_to_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_section
    ADD CONSTRAINT pt_section_stop_to_fkey FOREIGN KEY (stop_to) REFERENCES pt_stop(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_stop_parent_station_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_stop
    ADD CONSTRAINT pt_stop_parent_station_fkey FOREIGN KEY (parent_station) REFERENCES pt_stop(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_stop_road_section_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_stop
    ADD CONSTRAINT pt_stop_road_section_id_fkey FOREIGN KEY (road_section_id) REFERENCES road_section(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_stop_time_stop_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_stop_time
    ADD CONSTRAINT pt_stop_time_stop_id_fkey FOREIGN KEY (stop_id) REFERENCES pt_stop(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_stop_time_trip_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_stop_time
    ADD CONSTRAINT pt_stop_time_trip_id_fkey FOREIGN KEY (trip_id) REFERENCES pt_trip(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_trip_route_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_trip
    ADD CONSTRAINT pt_trip_route_id_fkey FOREIGN KEY (route_id) REFERENCES pt_route(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: pt_zone_network_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY pt_zone
    ADD CONSTRAINT pt_zone_network_id_fkey FOREIGN KEY (network_id) REFERENCES pt_network(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: road_restriction_time_penalty_period_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY road_restriction_time_penalty
    ADD CONSTRAINT road_restriction_time_penalty_period_id_fkey FOREIGN KEY (period_id) REFERENCES road_validity_period(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: road_restriction_time_penalty_restriction_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY road_restriction_time_penalty
    ADD CONSTRAINT road_restriction_time_penalty_restriction_id_fkey FOREIGN KEY (restriction_id) REFERENCES road_restriction(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: road_restriction_toll_period_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY road_restriction_toll
    ADD CONSTRAINT road_restriction_toll_period_id_fkey FOREIGN KEY (period_id) REFERENCES road_validity_period(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: road_restriction_toll_restriction_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY road_restriction_toll
    ADD CONSTRAINT road_restriction_toll_restriction_id_fkey FOREIGN KEY (restriction_id) REFERENCES road_restriction(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: road_section_node_from_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY road_section
    ADD CONSTRAINT road_section_node_from_fkey FOREIGN KEY (node_from) REFERENCES road_node(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: road_section_node_to_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY road_section
    ADD CONSTRAINT road_section_node_to_fkey FOREIGN KEY (node_to) REFERENCES road_node(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: road_section_speed_period_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY road_section_speed
    ADD CONSTRAINT road_section_speed_period_id_fkey FOREIGN KEY (period_id) REFERENCES road_validity_period(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: road_section_speed_road_section_id_fkey; Type: FK CONSTRAINT; Schema: tempus; Owner: -
--

ALTER TABLE ONLY road_section_speed
    ADD CONSTRAINT road_section_speed_road_section_id_fkey FOREIGN KEY (road_section_id) REFERENCES road_section(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--

