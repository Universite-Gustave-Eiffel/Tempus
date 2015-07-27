-- Add some two-way roads with distinct road section for each part
insert into tempus.road_section (id, road_type, node_from, node_to, traffic_rules_ft, traffic_rules_tf, length, car_speed_limit, road_name, roundabout, bridge, tunnel, ramp, tollway)
select (select max(id) from tempus.road_section)+row_number() over (), road_type, node_to, node_from, traffic_rules_tf, traffic_rules_ft, length*2, car_speed_limit/2., road_name,
'f', 'f', 'f', 'f', 'f'
from tempus.road_section where traffic_rules_tf = 1 and traffic_rules_ft != traffic_rules_tf
limit 10;
