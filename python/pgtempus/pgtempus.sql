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
if 'tempus_request' in SD:
    tempus_request = SD['tempus_request']
else:
    import sys
    addp = '/home/hme/src/tempus/src/python'
    if addp not in sys.path:
        sys.path.append(addp)
    import tempus_request
    SD['tempus_request'] = tempus_request
r = tempus_request.TempusRequest(wps)
return [p.name for p in r.plugin_list()]
$$ language plpythonu;

--
-- Call the isochrone plugin
--
drop function if exists tempus_wps.isochrone(double precision, double precision, real, timestamp, int[], text);
create function tempus_wps.isochrone(x double precision, y double precision, limit_value real, dt timestamp = now(), transport_modes int[] = '{1}', wps text ='http://localhost/wps')
returns table(x double precision, y double precision, mode smallint, cost real)
as $$
if 'tempus_request' in SD:
    tempus_request = SD['tempus_request']
else:
    import sys
    addp = '/home/hme/src/tempus/src/python'
    if addp not in sys.path:
        sys.path.append(addp)
    import tempus_request
    SD['tempus_request'] = tempus_request
import datetime
ddt = tempus_request.DateTime.from_dt(datetime.datetime.strptime(dt, '%Y-%m-%d %H:%M:%S'))
r = tempus_request.TempusRequest(wps)
r.request(plugin_name='isochrone_plugin', \
          plugin_options={'Isochrone/limit': limit_value}, \
          origin = tempus_request.Point(x,y), \
          steps = [ tempus_request.RequestStep(destination = tempus_request.Point(0,0), \
                                               constraint = tempus_request.Constraint( date_time = ddt, type = 2  )) ], \
          criteria = [tempus_request.Cost.Duration], \
          allowed_transport_modes = transport_modes)
return r.results[0].points
$$ language plpythonu;

select tempus_wps.plugin_list();
select tempus_wps.plugin_list();

drop table if exists t;
create table t as
select st_concavehull(st_collect(st_makepoint(x,y, 4326)), 0.95) as geom from tempus_wps.isochrone(-1.546040, 47.199764, 30.0, '2014-06-10 09:00'::timestamp, '{1,5,6}'::int[]);

