   -- Tempus - Multinet SQL import Wrapper
-- Licence MIT
-- Copyright Oslandia 2012

-- TABLE road_node 
INSERT INTO tempus.road_node

SELECT DISTINCT 
	jc.id,
	jc.jncttyp = 0 AS junction,
	jc.jncttyp = 2 AS bifurcation,
	ST_Force_3DZ(ST_SetSRID(geom, 2154)) AS geom
	-- FIXME change ST_SetSRID to ST_Transform as soon as loader will handle mandatory srid
FROM _tempus_import.jc AS jc
WHERE jc.feattyp = 4120;



-- TABLE road_section 

-- Begin to remove all related constraints and index (performances concern)
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_node_from_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_node_to_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_road_type_fkey;
ALTER TABLE tempus.poi DROP CONSTRAINT poi_road_section_id_fkey;
ALTER TABLE tempus.pt_stop DROP CONSTRAINT pt_stop_road_section_id_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_pkey;

-- create index to speed up next query
analyze _tempus_import.sr;
analyze _tempus_import.nw;
create index idx_tempus_import_sr_id on _tempus_import.sr (id);
create index idx_tempus_import_nw_id on _tempus_import.nw (id);

-- Proceed to INSERT
insert into tempus.road_section
SELECT 
	nw.id,

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

	case
		when oneway is null and frc <= 2 then 1
		when oneway is null then 512 + 4 + 2 + 1 -- [and frc > 2]
		when oneway = 'N' then 512 + 2
		when oneway = 'TF' then 2 -- [and test is true]
		when frc <= 2 then 1 -- [and oneway = 'FT' and test is true]
		else 512 + 4 + 2 + 1 -- [and oneway = 'FT' and test is true and frc > 2]
	end as transport_type_ft
	, case
		when oneway is null and frc <= 2 then 1
		when oneway is null then 512 + 4 + 2 + 1 -- [and frc > 2]
		when oneway = 'N' then 512 + 2
		when oneway = 'FT' then 2 -- [and test is false]
		when frc <= 2 then 1 -- [and oneway = 'TF' and test is false]
		else 512 + 4 + 2 + 1 -- [and oneway = 'TF' and test is false and frc > 2]
	end as transport_type_tf

	, meters AS length
	, speed.car_speed_limit
	, CASE speedcat
		WHEN 1 THEN 130 
		WHEN 2 THEN 115
		WHEN 3 THEN 95
		WHEN 4 THEN 80
		WHEN 5 THEN 60
		WHEN 6 THEN 40
		WHEN 7 THEN 20
		WHEN 8 THEN 8
		ELSE NULL 
	END AS car_average_speed,
	
	"name" AS road_name,

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

	CASE tollrd
		WHEN 'B' THEN true
		WHEN 'FT' THEN true
		WHEN 'TF' THEN true
		ELSE false
	END AS tollway,

	ST_SetSRID(ST_Force_3DZ(ST_LineMerge(nw.geom)), 2154) AS geom
	-- FIXME remove ST_LineMerge call as soon as loader will use Simple geometry option
	-- FIXME change ST_SetSRID to ST_Transform as soon as loader will handle mandatory srid

FROM 
	_tempus_import.nw AS nw
left join (
	select 
		sr.id
		, min(speed) as car_speed_limit 
	FROM 
		_tempus_import.sr 
	group by 
		sr.id
) as speed 
on 
	nw.id=speed.id
WHERE 
	nw.feattyp = 4110;

-- Restore constraints and index
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_pkey
	PRIMARY KEY (id);
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_node_from_fkey 
	FOREIGN KEY (node_from) REFERENCES tempus.road_node;
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_node_to_fkey
	FOREIGN KEY (node_to) REFERENCES tempus.road_node(id);
ALTER TABLE tempus.road_section ADD CONSTRAINT road_section_road_type_fkey
	FOREIGN KEY (road_type) REFERENCES tempus.road_type(id);
ALTER TABLE tempus.poi ADD CONSTRAINT poi_road_section_id_fkey
	FOREIGN KEY (road_section_id) REFERENCES tempus.road_section;
ALTER TABLE tempus.pt_stop ADD CONSTRAINT pt_stop_road_section_id_fkey
	FOREIGN KEY (road_section_id) REFERENCES tempus.road_section;


-- TABLE road_road
INSERT INTO tempus.road_road
        SELECT  t.id,
                t.road_section,
                (SELECT SUM(ST_Length(geom)) FROM _tempus_import.nw AS nw WHERE nw.id = ANY (t.road_section)) AS cost

        FROM (
        	SELECT   id,
                 array(
			SELECT trpelid FROM _tempus_import.mp AS a
                      	WHERE mp.id=a.id AND trpeltyp=4110 ORDER BY seqnr
                        ) as road_section

        FROM _tempus_import.mp AS mp
        WHERE mp.trpelid IN (SELECT nw.id FROM _tempus_import.nw AS nw) AND seqnr > 1
        GROUP BY id ) AS t;
