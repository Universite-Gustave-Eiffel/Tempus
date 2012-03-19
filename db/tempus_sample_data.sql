--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

SET search_path = tempus, pg_catalog;

--
-- Data for Name: road_node; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO road_node (id, junction, bifurcation) VALUES (1, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (2, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (3, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (4, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (5, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (6, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (7, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (8, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (9, false, false);
INSERT INTO road_node (id, junction, bifurcation) VALUES (10, false, false);


--
-- Data for Name: road_section; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO road_section (id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, car_average_speed, bus_average_speed, road_name, address_left_side, address_right_side, lane, roundabout, bridge, tunnel, ramp, tollway) VALUES (1, 4, 1, 2, 413, 413, 0.400000000000000022, 50, NULL, NULL, 'Rue A', NULL, NULL, 1, false, false, false, false, false);
INSERT INTO road_section (id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, car_average_speed, bus_average_speed, road_name, address_left_side, address_right_side, lane, roundabout, bridge, tunnel, ramp, tollway) VALUES (2, 4, 2, 3, 413, 413, 0.400000000000000022, 50, NULL, NULL, 'Rue B', NULL, NULL, 1, false, false, false, false, false);
INSERT INTO road_section (id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, car_average_speed, bus_average_speed, road_name, address_left_side, address_right_side, lane, roundabout, bridge, tunnel, ramp, tollway) VALUES (3, 4, 4, 5, 413, 413, 0.299999999999999989, 50, NULL, NULL, 'Rue C', NULL, NULL, 1, false, false, false, false, false);
INSERT INTO road_section (id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, car_average_speed, bus_average_speed, road_name, address_left_side, address_right_side, lane, roundabout, bridge, tunnel, ramp, tollway) VALUES (4, 4, 7, 8, 413, 413, 1.30000000000000004, 50, NULL, NULL, 'Rue E', NULL, NULL, 1, false, false, false, false, false);
INSERT INTO road_section (id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, car_average_speed, bus_average_speed, road_name, address_left_side, address_right_side, lane, roundabout, bridge, tunnel, ramp, tollway) VALUES (5, 4, 8, 10, 413, 413, 2.29999999999999982, 50, NULL, NULL, 'Rue F', NULL, NULL, 1, false, false, false, false, false);



--
-- Data for Name: poi; Type: TABLE DATA; Schema: tempus; Owner: hme
--



--
-- Data for Name: pt_calendar; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO pt_calendar (id, monday, tuesday, wednesday, thursday, friday, saturday, sunday, start_date, end_date) VALUES (1, true, true, true, true, true, true, true, '2012-01-01', '2012-12-31');


--
-- Data for Name: pt_calendar_date; Type: TABLE DATA; Schema: tempus; Owner: hme
--



--
-- Data for Name: pt_fare_attribute; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO pt_fare_attribute (id, price, currency_type, transfers, transfers_duration) VALUES (1, 1.5, 'EUR', -1, 3600);


--
-- Data for Name: pt_network; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO pt_network (id, name) VALUES (1, 'TAN');


--
-- Data for Name: pt_route; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO pt_route (id, network_id, short_name, long_name, route_type) VALUES (1, 1, 'Ligne 2A', 'Ligne 2 - trajet primaire', 0);


--
-- Data for Name: pt_fare_rule; Type: TABLE DATA; Schema: tempus; Owner: hme
--



--
-- Data for Name: pt_trip; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO pt_trip (id, route_id, service_id, short_name) VALUES (1, 1, 1, 'TAN2AM');
INSERT INTO pt_trip (id, route_id, service_id, short_name) VALUES (2, 1, 1, 'TAN2AM2');
INSERT INTO pt_trip (id, route_id, service_id, short_name) VALUES (3, 1, 1, 'TANAM3');


--
-- Data for Name: pt_frequency; Type: TABLE DATA; Schema: tempus; Owner: hme
--



--
-- Data for Name: pt_stop; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO pt_stop (id, name, location_type, parent_station, road_section_id, zone_id, abscissa_road_section) VALUES (1, 'Commerce', true, 1, 1, 0, NULL);
INSERT INTO pt_stop (id, name, location_type, parent_station, road_section_id, zone_id, abscissa_road_section) VALUES (2, 'Hôtel Dieu', true, 2, 2, 1, 0.200000000000000011);
INSERT INTO pt_stop (id, name, location_type, parent_station, road_section_id, zone_id, abscissa_road_section) VALUES (3, 'Aimé Delrue', true, 3, 3, 1, 0.299999999999999989);
INSERT INTO pt_stop (id, name, location_type, parent_station, road_section_id, zone_id, abscissa_road_section) VALUES (4, 'Wattignies', true, 4, 4, 1, 0.299999999999999989);
INSERT INTO pt_stop (id, name, location_type, parent_station, road_section_id, zone_id, abscissa_road_section) VALUES (5, 'Mangin', true, 5, 5, 1, 0.299999999999999989);


--
-- Data for Name: pt_section; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO pt_section (stop_from, stop_to, network_id) VALUES (1, 2, 1);
INSERT INTO pt_section (stop_from, stop_to, network_id) VALUES (2, 3, 1);
INSERT INTO pt_section (stop_from, stop_to, network_id) VALUES (3, 4, 1);
INSERT INTO pt_section (stop_from, stop_to, network_id) VALUES (4, 5, 1);


--
-- Data for Name: pt_stop_time; Type: TABLE DATA; Schema: tempus; Owner: hme
--

INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (4, 1, '10:06:00', '10:06:00', 4, 4, '2A Direction Mangin', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (5, 1, '10:08:00', '10:08:00', 5, 5, '2A Direction Mangin', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (10, 2, '10:10:00', '10:10:00', 1, 1, '2A Direction Mangin', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (11, 2, '10:12:00', '10:12:00', 2, 2, '2A Direction Mangin', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (12, 2, '10:14:00', '10:14:00', 3, 3, '2A Direction Mangin', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (13, 2, '10:16:00', '10:16:00', 4, 4, '2A Direction Mangin', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (14, 2, '10:18:00', '10:18:00', 5, 5, '2A Direction Mangin', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (1, 3, '10:00:00', '10:00:00', 1, 1, '3 Direction Rezé', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (2, 3, '10:02:00', '10:02:00', 2, 2, '3 Direction Rezé', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (3, 3, '10:04:00', '10:04:00', 3, 3, '3 Direction Rezé', 0, 0, 0);
INSERT INTO pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) VALUES (15, 1, '10:05:00', '10:05:00', 3, 3, '2A Direction Mangin', 0, 0, 0);


--
-- Data for Name: pt_transfer; Type: TABLE DATA; Schema: tempus; Owner: hme
--



--
-- Data for Name: road_road; Type: TABLE DATA; Schema: tempus; Owner: hme
--



--
-- PostgreSQL database dump complete
--

