--
-- clean up a road network

-- remove cycles
-- choose a unique edge between parallel edges of the same orientation
-- WARNING : it will choose an arbitrary edge among all the possible ones. Consider rather a manual cleaning

delete from tempus.road_section as rs
where not exists ( -- not exists is faster than NOT IN here
      select * from (
        select distinct on (node_from, node_to) id -- distinct : choose one parallel edge
        from
                tempus.road_section
        where node_from != node_to -- get rid of cycles
     ) as t
     where t.id = rs.id
);

--
-- from pairs of opposite edges, choose the smallest one
-- WARNING : this is may be more destructive than what you want
delete from tempus.road_section
where id in (
select
        -- delete the longest road
        case when st_length(rs1.geom) > st_length(rs2.geom) then rs1.id else rs2.id end
from
        tempus.road_section as rs1
left join
        tempus.road_section as rs2
on
        rs1.node_from = rs2.node_to
and
        rs1.node_to = rs2.node_from
where
        rs2.id is not null
and
        rs1.id > rs2.id -- avoid to remove two times the same id
); 

-- Delete turning restrictions not referenced in existing road_sections
delete from tempus.road_restriction_time_penalty where restriction_id not in
(
SELECT road_restriction.id
  FROM tempus.road_restriction, ( select array_agg(id) as id from tempus.road_section ) q
  WHERE sections <@ q.id
); 

delete from tempus.road_restriction where id not in
(
SELECT road_restriction.id
  FROM tempus.road_restriction, ( select array_agg(id) as id from tempus.road_section ) q
  WHERE sections <@ q.id
); 
