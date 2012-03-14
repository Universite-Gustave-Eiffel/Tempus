
--
-- Data for Name: road_node; Type: TABLE DATA; Schema: tempus; Owner: hme
--

COPY road_node (id, junction, bifurcation, geom) FROM stdin;
1	f	f	\N
2	f	f	\N
3	f	f	\N
4	f	f	\N
5	f	f	\N
6	f	f	\N
7	f	f	\N
8	f	f	\N
9	f	f	\N
10	f	f	\N
\.


--
-- Data for Name: road_section; Type: TABLE DATA; Schema: tempus; Owner: hme
--

COPY road_section (id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, car_average_speed, bus_average_speed, road_name, address_left_side, address_right_side, lane, roundabout, bridge, tunnel, ramp, tollway, geom) FROM stdin;
1	4	1	2	413	413	0.400000000000000022	50	\N	\N	Rue A	\N	\N	1	f	f	f	f	f	\N
2	4	2	3	413	413	0.400000000000000022	50	\N	\N	Rue B	\N	\N	1	f	f	f	f	f	\N
3	4	4	5	413	413	0.299999999999999989	50	\N	\N	Rue C	\N	\N	1	f	f	f	f	f	\N
4	4	7	8	413	413	1.30000000000000004	50	\N	\N	Rue E	\N	\N	1	f	f	f	f	f	\N
5	4	8	10	413	413	2.29999999999999982	50	\N	\N	Rue F	\N	\N	1	f	f	f	f	f	\N
\.

--
-- Data for Name: pt_network; Type: TABLE DATA; Schema: tempus; Owner: hme
--

COPY pt_network (id, name) FROM stdin;
1	TAN
\.


--
-- Data for Name: pt_stop; Type: TABLE DATA; Schema: tempus; Owner: hme
--

COPY pt_stop (id, name, location_type, parent_station, road_section_id, zone_id, abscissa_road_section, geom) FROM stdin;
1	Commerce	t	1	1	0	\N	\N
2	Hôtel Dieu	t	2	2	1	0.200000000000000011	\N
3	Aimé Delrue	t	3	3	1	0.299999999999999989	\N
4	Wattignies	t	4	4	1	0.299999999999999989	\N
5	Mangin	t	5	5	1	0.299999999999999989	\N
\.


--
-- Data for Name: pt_section; Type: TABLE DATA; Schema: tempus; Owner: hme
--

COPY pt_section (stop_from, stop_to, network_id, geom) FROM stdin;
1	2	1	\N
2	3	1	\N
3	4	1	\N
4	5	1	\N
\.

--
-- Data for Name: pt_route; Type: TABLE DATA; Schema: tempus; Owner: hme
--

COPY pt_route (id, network_id, short_name, long_name, route_type, geom) FROM stdin;
1	1	Ligne 2A	Ligne 2 - trajet primaire	0	\N
\.


--
-- Data for Name: pt_trip; Type: TABLE DATA; Schema: tempus; Owner: hme
--

COPY pt_trip (id, route_id, service_id, short_name) FROM stdin;
1	1	1	TAN2AM
2	1	1	TAN2AM2
\.


--
-- Data for Name: pt_stop_time; Type: TABLE DATA; Schema: tempus; Owner: hme
--

COPY pt_stop_time (id, trip_id, arrival_time, departure_time, stop_id, stop_sequence, stop_headsign, pickup_type, drop_off_type, shape_dist_traveled) FROM stdin;
1	1	10:00:00	10:00:00	1	1	2A Direction Mangin	0	0	0
2	1	10:02:00	10:02:00	2	2	2A Direction Mangin	0	0	0
3	1	10:04:00	10:04:00	3	3	2A Direction Mangin	0	0	0
4	1	10:06:00	10:06:00	4	4	2A Direction Mangin	0	0	0
5	1	10:08:00	10:08:00	5	5	2A Direction Mangin	0	0	0
10	2	10:10:00	10:10:00	1	1	2A Direction Mangin	0	0	0
11	2	10:12:00	10:12:00	2	2	2A Direction Mangin	0	0	0
12	2	10:14:00	10:14:00	3	3	2A Direction Mangin	0	0	0
13	2	10:16:00	10:16:00	4	4	2A Direction Mangin	0	0	0
14	2	10:18:00	10:18:00	5	5	2A Direction Mangin	0	0	0
\.

