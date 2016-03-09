/*

POIs import

POI import needs the following templated fields :
poi_type: integer, type of the POI (1 - car park, 4 - shared cycle point, etc.), default to 5 (user POI)
name : string, field containing the name of each POI, default to 'pname'
service_name : string, name of the service (Bicloo, Velov, Marguerite)
filter: string, WHERE clause of the import, default to 'true' (no filter)

*/

BEGIN;

INSERT INTO tempus.transport_mode (id, name,public_transport,traffic_rules,shared_vehicle,return_shared_vehicle,need_parking)
   SELECT
   (SELECT max(id)+1 FROM tempus.transport_mode), 
   CASE WHEN %(poi_type) = 2 THEN
        'Shared car (%(service_name))'
        WHEN %(poi_type) = 4 THEN
        'Shared bike (%(service_name))'
   END,
   'f', -- public transport
   CASE WHEN %(poi_type) = 2 THEN 4 ELSE 2 END, -- traffic_rules
   't', -- shared_vehicule
   't', -- return_shared_vehicle
   't' -- need_parking
   WHERE %(poi_type) in (2,4);

drop sequence if exists _tempus_import.poi_id;
create sequence _tempus_import.poi_id start with 1;
select setval('_tempus_import.poi_id', (select case when max(id) is null then 1 else max(id)+1 end from tempus.poi), false);

insert into
    tempus.poi (
            id
            , poi_type
            , name
            , parking_transport_modes
            , road_section_id
            , abscissa_road_section
            , geom
        )
select
    nextval('_tempus_import.poi_id')::bigint as id
    , %(poi_type)::integer as poi_type
    , %(name) as name
    , array[(select
    case
        when %(poi_type) = 1 -- car park
             then 3 -- private car
        when %(poi_type) = 2 -- shared car
             then (select max(id) from tempus.transport_mode) -- last inserted transport mode
        when %(poi_type) = 3 -- bike
             then 2 -- private bike
        when %(poi_type) = 4 -- shared bike
             then (select max(id) from tempus.transport_mode) -- last inserted transport mode
        else
             null
    end)]
    , road_section_id
    , st_LineLocatePoint(geom_road, pgeom)::double precision as abscissa_road_section
    , st_Force3DZ(pgeom)
from (
    select
        poi.*
        , poi.geom as pgeom
        , first_value(rs.id) over nearest as road_section_id
        , first_value(rs.geom) over nearest as geom_road
        , row_number() over nearest as nth
     from
        _tempus_import.poi as poi
     join
        tempus.road_section as rs
     on
		-- only consider road sections within xx meters
		-- poi further than this distance will not be included
        st_dwithin(poi.geom, rs.geom, 30)
     where
        %(filter)
    window
        -- nearest row geometry for each poi
        nearest as (partition by poi.gid order by st_distance(poi.geom, rs.geom))
) as poi_ratt
where
    -- only take one rattachment
    nth = 1
    ;
    
COMMIT;
