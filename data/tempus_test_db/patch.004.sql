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

refresh materialized view tempus.pt_service_day;

refresh materialized view tempus.pt_timetable;
