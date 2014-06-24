/*

POIs import

POI import needs the following templated fields :
poi_type: integer, type of the POI (1 - car park, 4 - shared cycle point, etc.), default to 5 (user POI)
name : string, field containing the name of each POI, default to 'pname'
parking_transport_type : integer, type of parking, default: computed if poi_type is given
filter: string, WHERE clause of the import, default to 'true' (no filter)

*/

insert into
    tempus.poi (
            id
            , poi_type
            , pname
            , parking_transport_type
            , road_section_id
            , abscissa_road_section
            , geom
        )
select
    gid as id
    , %(poi_type)::integer as poi_type
    , '%(name)' as pname
    , %(parking_transport_type) as parking_transport_type
    , road_section_id
    , st_line_locate_point(geom_road, pgeom)::double precision as abscissa_road_section
    , st_force_3dz(pgeom)
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
    
