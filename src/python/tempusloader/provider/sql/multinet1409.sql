-- Tempus - Multinet SQL import Wrapper

-- TABLE road_node
INSERT INTO tempus.road_node
SELECT DISTINCT
        jc.id,
        jc.jncttyp = 2 AS bifurcation,
--  FIXED elev attribut must be considered in 3D
        ST_Translate( ST_Force3D(st_transform(geom, %(native_srid) )), 0, 0, elev ) AS geom
FROM _tempus_import.jc AS jc
WHERE jc.feattyp = 4120; -- 4120 means road node, 4220 means rail node



-- TABLE road_section

-- Begin to remove all related constraints and index (performances concern)
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_node_from_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_node_to_fkey;
ALTER TABLE tempus.poi DROP CONSTRAINT poi_road_section_id_fkey;
ALTER TABLE tempus.pt_stop DROP CONSTRAINT pt_stop_road_section_id_fkey;
ALTER TABLE tempus.road_section_speed DROP CONSTRAINT road_section_speed_road_section_id_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_pkey;

-- create index to speed up next query
ANALYSE _tempus_import.sr;
ANALYSE _tempus_import.nw;
CREATE INDEX idx_tempus_import_sr_id ON _tempus_import.sr (id);
CREATE INDEX idx_tempus_import_nw_id ON _tempus_import.nw (id);

-- Proceed to INSERT
INSERT INTO tempus.road_section
SELECT
        nw.id,

        -- FIXME use net2class as road_type
        CASE frc
                WHEN 0 THEN 1
                WHEN 1 THEN 2
                WHEN 2 THEN 2
                WHEN 3 THEN 3
                WHEN 4 THEN 3
                WHEN 5 THEN 3
                WHEN 6 THEN 4
                WHEN 7 THEN 5
                WHEN 8 THEN 7
                ELSE NULL
        END AS road_type,
        f_jnctid AS node_from,
        t_jnctid AS node_to,


--    TrafficRulePedestrian      = 1,
--    TrafficRuleBicycle         = 2,
--    TrafficRuleCar             = 4,
--    TrafficRuleTaxi            = 8,
--    TrafficRuleCarPool         = 16,
--    TrafficRuleTruck           = 32,
--    TrafficRuleCoach           = 64,
--    Special traffic rule for public transport : 256

        CASE
                -- FIXED FRC not pertinent; freeway (= 1) more efficient for motorways and freeways (Separated Grade Crossing)
                WHEN oneway IS NULL AND freeway = 1 THEN 0 + 0 + 4 + 8 + 16 + 32 + 64   -- Pedestrian + Bicycle excluded 
                WHEN oneway IS NULL                 THEN 1 + 2 + 4 + 8 + 16 + 32 + 64   -- [and freeway = 0]
                WHEN oneway = 'N'  OR FOW = 19      THEN 1
                WHEN oneway = 'TF'                  THEN 1                              -- [and test is true]
                WHEN freeway = 1                    THEN 0 + 0 + 4 + 8 + 16 + 32 + 64   -- [and oneway = 'FT' and test is true]
                ELSE 1 + 2 + 4 + 8 + 16 + 32 + 64                                       -- [and oneway = 'FT' and test is true and frc > 2]
        END AS traffic_rules_ft,

        CASE
                WHEN oneway IS NULL AND freeway = 1 THEN 0 + 0 + 4 + 8 + 16 + 32 + 64   -- Pedestrian + Bicycle excluded 
                WHEN oneway IS NULL                 THEN 1 + 2 + 4 + 8 + 16 + 32 + 64   -- [and freeway = 0]
                WHEN oneway = 'N' OR FOW = 19       THEN 1
                WHEN oneway = 'FT'                  THEN 1                              -- [and test is false]
                WHEN freeway = 1                    THEN 0 + 0 + 4 + 8 + 16 + 32 + 64   -- [and oneway = 'TF' and test is false]
                ELSE 1 + 2 + 4 + 8 + 16 + 32 + 64                                       -- [and oneway = 'TF' and test is false and frc > 2]
        END AS traffic_rules_tf,
        meters AS length,

        -- FIXME if country in (GBR, USA,...) speed limit in mph (not km/h)
        speed.car_speed_limit,
        nw.name as name,

        CASE lanes
                WHEN 0 THEN NULL
                ELSE lanes
        END AS lane,

        CASE fow
                WHEN 4 THEN true
                ELSE false
        END AS roundabout,

        CASE partstruc
                WHEN 2 THEN true
                ELSE false
        END AS bridge,

        CASE partstruc
                WHEN 1 THEN true
                ELSE false
        END AS tunnel,

        CASE ramp
                WHEN 0 THEN false
                WHEN 1 THEN true
                WHEN 2 THEN true
                ELSE NULL
        END AS ramp,

        CASE
                WHEN tollrd is null THEN false
                WHEN tollrd = 0 THEN false
                ELSE true
        END AS tollway,

        ST_Transform(ST_Force3D(ST_LineMerge(nw.geom)), %(native_srid)) AS geom
        -- FIXME remove ST_LineMerge call as soon as loader will use Simple geometry option
        -- FIXME elev attribut must be considered in 3D

FROM
        _tempus_import.nw AS nw
left join (
        select
                sr.id,
                min(speed) as car_speed_limit
        FROM
                  _tempus_import.sr     sr
        LEFT JOIN _tempus_import.st     st ON sr.id = st.id AND sr.seqnr = st.seqnr
        WHERE   
                -- FIXED Only mandatory speed limit (not recommended) and for Passenger Cars (or all vehicle) are used
                speedtyp = '1' AND vt in ( 0, 11 )
                -- but temporary Speed limits are NOT used
                AND st.timedom is null
        group by
                sr.id
) as speed
on
        nw.id=speed.id
WHERE
        nw.feattyp = 4110; -- 4110 : road element, 4130 : ferry transfer element


-- Removing vehicles not allowed to go through the positive direction (ft)
--    TrafficRulePedestrian      = 1,
--    TrafficRuleBicycle         = 2,
--    TrafficRuleCar             = 4,
--    TrafficRuleTaxi            = 8,
--    TrafficRuleCarPool         = 16,
--    TrafficRuleTruck           = 32,
--    TrafficRuleCoach           = 64,
--    Special traffic rule for public transport : 256
UPDATE tempus.road_section
SET traffic_rules_ft = traffic_rules_ft -
        (traffic_rules_ft & ( 
                CASE 
                        WHEN ARRAY[0::smallint] <@ array_agg then 1 + 2 + 4 + 8 + 16 + 32 + 64  -- All vehicles
                ELSE 
                          CASE WHEN ARRAY[11::smallint] <@ array_agg THEN 4 + 16 ELSE 0 END     -- Passenger Cars
                        + CASE WHEN ARRAY[16::smallint] <@ array_agg THEN 8      ELSE 0 END     -- Taxi
-- FIXME Buses  + CASE WHEN ARRAY[17::smallint] <@ array_agg THEN ?? ELSE 0 END -- Public Bus
                        + CASE WHEN ARRAY[24::smallint] <@ array_agg THEN 2      ELSE 0 END     -- Bicycle
                END ) )
FROM
(
        SELECT RS.id, array_agg(vt ORDER BY vt)
        FROM            _tempus_import.rs       RS
        -- FIXME must not consider temporary restrictions
        LEFT JOIN       _tempus_import.td       TD ON RS.id = TD.id AND RS.seqnr = TD.seqnr
        -- FIXED dir_pos and not restrval (null)
        -- dir_pos : 1 = both direction ; 2 = positive direction (FT)
        WHERE           feattyp = 4110 AND restrtyp = 'DF' AND dir_pos in ( 1, 2 )
        -- FIXED temporary restrictions are NOT used
        AND             TD.timedom is null
        GROUP BY RS.id
) q
WHERE q.id = road_section.id ;

-- Removing vehicles not allowed to go through the negative direction (tf)
UPDATE tempus.road_section
SET traffic_rules_tf = traffic_rules_tf -
        (traffic_rules_tf & (  -- FIXED traffic_rules_tf and NOT traffic_rules_ft
                CASE    WHEN ARRAY[0::smallint] <@ array_agg then 1 + 2 + 4 + 8 + 16 + 32 + 64  -- All vehicles
                ELSE
                          CASE WHEN ARRAY[11::smallint] <@ array_agg THEN 4 + 16 ELSE 0 END     -- Passenger Cars
                        + CASE WHEN ARRAY[16::smallint] <@ array_agg THEN 8      ELSE 0 END     -- Taxi
-- FIXME Buses  + CASE WHEN ARRAY[17::smallint] <@ array_agg THEN ?? ELSE 0 END -- Public Bus
                        + CASE WHEN ARRAY[24::smallint] <@ array_agg THEN 2      ELSE 0 END     -- Bicycle
        END ) )
FROM
(
        SELECT RS.id, array_agg(vt ORDER BY vt)
        FROM _tempus_import.rs       RS
        -- FIXED must not consider temporary restrictions
        LEFT JOIN       _tempus_import.td       TD ON RS.id = TD.id AND RS.seqnr = TD.seqnr
        -- FIXED dir_pos and not restrval (null)
        -- dir_pos : 1 = both direction ; 3 = negative direction (TF)
        WHERE           feattyp = 4110 AND restrtyp = 'DF' AND dir_pos in ( 1, 3 )
        -- FIXED temporary restrictions are NOT used
        AND             TD.timedom is null
        GROUP BY RS.id 
) q
WHERE q.id = road_section.id ;

-- cleanup border lines

-- delete every section with an unknown node_from
delete from tempus.road_section
where id in
(select
        rs.id
from
        tempus.road_section as rs
left join
        tempus.road_node as rn
on
        rs.node_from = rn.id
where
        rn.id is null
);
-- delete every section with an unknown node_to
delete from tempus.road_section
where id in
(select
        rs.id
from
        tempus.road_section as rs
left join
        tempus.road_node as rn
on
        rs.node_to = rn.id
where
        rn.id is null
);

-- Restore constraints and index
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_pkey
        PRIMARY KEY (id);
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_node_from_fkey
        FOREIGN KEY (node_from) REFERENCES tempus.road_node;
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_node_to_fkey
        FOREIGN KEY (node_to) REFERENCES tempus.road_node(id);
ALTER TABLE tempus.poi ADD CONSTRAINT poi_road_section_id_fkey
        FOREIGN KEY (road_section_id) REFERENCES tempus.road_section;
ALTER TABLE tempus.pt_stop ADD CONSTRAINT pt_stop_road_section_id_fkey
        FOREIGN KEY (road_section_id) REFERENCES tempus.road_section;

CREATE INDEX idx_road_node_geom ON tempus.road_node USING gist (geom);
CREATE INDEX idx_road_section_geom ON tempus.road_section USING gist (geom);

-- TABLE road_restriction
INSERT INTO tempus.road_restriction
SELECT
        mp.id::bigint as id,
        array_agg(trpelid::bigint order by seqnr) as road_section
FROM
        _tempus_import.mp as mp
LEFT JOIN
        _tempus_import.mn as mn
ON
        mn.id = mp.id
WHERE
        mn.feattyp in (2101,2103)
AND
        mp.trpeltyp = 4110
AND
        mn.promantyp = 0
GROUP BY mp.id;

INSERT INTO tempus.road_restriction_time_penalty
SELECT
        road_restriction.id as restriction_id,
        0 as period_id,
        CASE WHEN ARRAY[0::smallint] <@ array_agg then 1 + 2 + 4 + 8 + 16 + 32 + 64
        ELSE CASE WHEN ARRAY[11::smallint] <@ array_agg THEN 4 + 16 ELSE 0 END
                + CASE WHEN ARRAY[16::smallint] <@ array_agg THEN 8 ELSE 0 END
                + CASE WHEN ARRAY[24::smallint] <@ array_agg THEN 2 ELSE 0 END
        END
        AS traffic_rules,
        'Infinity'::float as cost
FROM
(
        SELECT RS.id, array_agg(vt order by vt)
        FROM            _tempus_import.rs       RS
        -- FIXED must not consider temporary restrictions
        LEFT JOIN       _tempus_import.td       TD ON RS.id = TD.id AND RS.seqnr = TD.seqnr
        WHERE           rs.feattyp in (2101, 2103) AND vt in (0, 11, 16, 24)
        -- FIXED temporary restrictions are NOT used
        AND             TD.timedom is null
        GROUP BY RS.id
) q, tempus.road_restriction
WHERE q.id = road_restriction.id;

-- TODO : add blocked passage (table rs, restrtyp = 'BP') => defined with an edge and a blocked extreme node (from_node if restrval = 1, to_node if restrval = 2)
-- Could be represented as a road_restriction composed of the edge and each adjacent edge from the chosen extreme node


-- Vacuuming database
VACUUM FULL ANALYSE;
