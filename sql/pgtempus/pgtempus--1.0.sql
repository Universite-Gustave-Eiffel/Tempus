-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pgtempus" to load this file. \quit

create extension if not exists plpythonu;

create schema tempus_wps;

--
-- Get the list of loaded plugins
--
create function tempus_wps.plugin_list(wps text ='http://localhost/wps') returns setof text
as $$
from pytempus import TempusRequest
r = TempusRequest(wps)
return [p.name for p in r.plugin_list()]
$$ language plpythonu;

--
-- Call the isochrone plugin
--
create function tempus_wps.isochrone(x double precision, y double precision, limit_value real,
                dt timestamp = now(), transport_modes int[] = '{1}', wps text ='http://localhost/wps',
                walking_speed_km_h real = 3.6,
                cycling_speed_km_h real = 12.0)
returns table(x double precision, y double precision, mode smallint, cost real)
as $$
from pytempus import TempusRequest, Point, RequestStep, Cost, Constraint, DateTime
import datetime
if '.' in dt:
   ddt = DateTime.from_dt(datetime.datetime.strptime(dt, '%Y-%m-%d %H:%M:%S.%f'))
else:
   ddt = DateTime.from_dt(datetime.datetime.strptime(dt, '%Y-%m-%d %H:%M:%S'))
r = TempusRequest(wps)
r.request(plugin_name='isochrone_plugin', \
          plugin_options={'Isochrone/limit': limit_value, \
                          'Time/walking_speed': walking_speed_km_h, \
                          'Time/cycling_speed': cycling_speed_km_h}, \
          origin = Point(x,y), \
          steps = [ RequestStep(destination = Point(0,0), \
                                               constraint = Constraint( date_time = ddt, type = 2  )) ], \
          criteria = [Cost.Duration], \
          allowed_transport_modes = transport_modes)
return r.results[0].points
$$ language plpythonu;

