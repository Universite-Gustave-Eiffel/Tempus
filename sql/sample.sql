select st_concavehull(st_collect(st_makepoint(x,y, 4326)), 0.95) as geom from tempus_wps.isochrone(-1.546040, 47.199764, 10.0, transport_modes := '{1}'::int[]);
select * from tempus_wps.isochrone(-1.546040, 47.199764, 10.0, transport_modes := '{1}'::int[], walking_speed = 4.0);
