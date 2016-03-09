--
-- clean up public transports

--
-- delete pt stops that are not used in any pt_section
delete from tempus.pt_stop
where id in 
(
  select p.id
  from
      tempus.pt_stop as p
  left join
      tempus.pt_section as s1
  on
      p.id = s1.stop_from
  left join
      tempus.pt_section as s2
  on
      p.id = s2.stop_to
  left join
      tempus.pt_stop_time
  on p.id = stop_id
  left join
      tempus.pt_stop as pp
  on p.id = pp.parent_station
  where
      s1.stop_from is null
  and s2.stop_to is null
  and stop_id is null
  and pp.parent_station is null
);
