create extension if not exists postgis;
create extension if not exists plpythonu;

drop schema if exists tempus_wps cascade;
create schema tempus_wps;

--
-- Get the list of loaded plugins
--
drop function if exists tempus_wps.plugin_list(text);
create function tempus_wps.plugin_list(wps text ='http://localhost/wps') returns setof text
as $$
from pytempus import TempusRequest
r = TempusRequest(wps)
return [p.name for p in r.plugin_list()]
$$ language plpythonu;

--
-- Call the isochrone plugin
--
drop function if exists tempus_wps.isochrone(double precision, double precision, real, timestamp, int[], text, real, real);
create function tempus_wps.isochrone(x double precision, y double precision, limit_value real,
                dt timestamp = now(), transport_modes int[] = '{1}', wps text ='http://localhost/wps',
                walking_speed_km_h real = 3.6,
                cycling_speed_km_h real = 12.0)
returns table(x double precision, y double precision, mode smallint, cost real)
as $$
from pytempus import TempusRequest, Point, RequestStep, Cost, Constraint, DateTime
import datetime
ddt = DateTime.from_dt(datetime.datetime.strptime(dt, '%Y-%m-%d %H:%M:%S.%f'))
r = TempusRequest(wps)
r.request(plugin_name='isochrone_plugin', \
          plugin_options={'Isochrone/limit': limit_value, 'Time/walking_speed': walking_speed_km_h, 'Time/cycling_speed': cycling_speed_km_h}, \
          origin = Point(x,y), \
          steps = [ RequestStep(destination = Point(0,0), \
                                               constraint = Constraint( date_time = ddt, type = 2  )) ], \
          criteria = [Cost.Duration], \
          allowed_transport_modes = transport_modes)
return r.results[0].points
$$ language plpythonu;

select tempus_wps.plugin_list();
select tempus_wps.plugin_list();

drop table if exists t;
create table t as
select st_concavehull(st_collect(st_makepoint(x,y, 4326)), 0.95) as geom from tempus_wps.isochrone(-1.546040, 47.199764, 10.0, transport_modes := '{1}'::int[]);

