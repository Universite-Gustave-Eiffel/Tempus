-- Tempus - Navteq SQL import Wrapper
-- Licence MIT
-- Copyright Oslandia 2012

-- Handle direction type
CREATE OR REPLACE FUNCTION _tempus_import.navteq_transport_direction(text, text, boolean)
RETURNS integer AS $$

DECLARE 
	tt integer;
BEGIN

	IF $1 = '1' OR $1 = '2'	 THEN tt := 1;		-- Only cars for motorway and primary road
	ELSE		     	      tt := 1 + 2 + 4 + 512;
	END IF; 

	IF     $2 IS NULL 					THEN RETURN tt;
	ELSIF  $2 = 'B'						THEN RETURN 2 + 512;
	ELSIF ($2 = 'F' AND $3) OR ($2 = 'T' AND NOT $3) 	THEN RETURN tt;
	ELSIF ($2 = 'T' AND $3) OR ($2 = 'F' AND NOT $3)	THEN RETURN 2;
	END IF;

	RETURN NULL;
END;
$$ LANGUAGE plpgsql;


-- TABLE road_node 
INSERT INTO tempus.road_node
	SELECT
                DISTINCT id,
                true as junction,
                false as bifurcation
        FROM 
		(SELECT ref_in_id AS id  FROM _tempus_import.streets
		 UNION
                 SELECT nref_in_id AS id FROM _tempus_import.streets) as t;

update tempus.road_node
set
        geom = ST_Force3DZ(ST_SetSRID(ST_StartPoint(st.geom),2154))
from
        _tempus_import.streets as st
where
        id = ref_in_id;

update tempus.road_node
set
        geom = ST_Force3DZ(ST_SetSRID(ST_EndPoint(st.geom),2154))
from
        _tempus_import.streets as st
where
        id = nref_in_id;


-- TABLE road_section 

-- Begin to remove all related constraints and index (performances concern)
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_node_from_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_node_to_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_road_type_fkey;
ALTER TABLE tempus.poi DROP CONSTRAINT poi_road_section_id_fkey;
ALTER TABLE tempus.pt_stop DROP CONSTRAINT pt_stop_road_section_id_fkey;
ALTER TABLE tempus.road_section DROP CONSTRAINT road_section_pkey;

-- Proceed to INSERT
INSERT INTO tempus.road_section 

SELECT 
	link_id AS id,

        CASE route_type 
		WHEN '1' THEN 1
		WHEN '2' THEN 1
		WHEN '3' THEN 2
                WHEN '4' THEN 3 
                WHEN '5' THEN 4 
		ELSE 5 -- others
	END AS road_type,

	ref_in_id AS node_from,
	nref_in_id AS node_to,

	_tempus_import.navteq_transport_direction(route_type, dir_travel, 't') AS transport_type_ft,
	_tempus_import.navteq_transport_direction(route_type, dir_travel, 'f') AS transport_type_tf,

	ST_Length(geom) AS length,
	fr_spd_lim AS car_speed_limit,

	CASE speed_cat::integer
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

	st_name AS road_name,

	CASE lane_cat::integer
		WHEN 0 THEN NULL
		WHEN 1 THEN 1
		WHEN 2 THEN 2
		WHEN 3 THEN 4
		ELSE NULL
	END AS lane,

	roundabout = 'Y' AS rounadbout,
	bridge = 'Y' AS bridge,
	tunnel = 'Y' AS tunnel,
	ramp = 'Y' AS ramp,
	tollway = 'Y' AS tollway,

	ST_SetSRID(ST_Force_3DZ(ST_LineMerge(geom)), 2154) AS geom
	-- FIXME remove ST_LineMerge call as soon as loader will use Simple geometry option
	-- FIXME change ST_SetSRID to ST_Transform as soon as loader will handle mandatory srid

FROM _tempus_import.streets AS st;

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


-- Set bifurcation flag
update tempus.road_node
set
        bifurcation = true,
        junction = false
where id in 
(
        select
                rn.id as id
        from
                tempus.road_node as rn,
                tempus.road_section as rs
        where
                rs.node_from = rn.id
        or
                rs.node_to = rn.id
        group by
	        rn.id
        having count(*) > 2
);


-- TABLE road_road
INSERT INTO tempus.road_road
select
	mcond_id as id,
	case when ar_auto = 'Y' then 1 else 0 end
	+ case when ar_bus = 'Y' then 8 else 0 end
	--+ case when ar_taxis = 'Y' then ?? else 0 end
	+ case when ar_carpool = 'Y' then 256 else 0 end
	+ case when ar_pedstrn = 'Y' then 2 else 0 end
	--+ case when ar_trucks = 'Y' then ?? else 0 end
	as transport_types,
	array_agg(link order by mseq_number) as road_section,
        -1 as road_cost
from
(

-- select first link id of the sequence
select
	*,
	cdms.cond_id::bigint as mcond_id,
	rdms.link_id::bigint as link,
	0 as mseq_number -- first item of the sequence
from
	_tempus_import.cdms as cdms
join
	_tempus_import.rdms as rdms
on
	cdms.cond_id = rdms.cond_id
where
	cond_type = 7
union

-- union with remaining link ids
select
	*,
	cdms.cond_id::bigint as mcond_id,
	man_linkid::bigint as link,
	seq_number as mseq_number -- here seq_number >= 1
from
	_tempus_import.cdms as cdms
join
	_tempus_import.rdms as rdms
on
	cdms.cond_id = rdms.cond_id
where
	cond_type = 7
) as t
group by
	mcond_id,
	ar_auto,
	ar_bus,
	ar_carpool,
	ar_pedstrn
;
       

-- Remove import function (direction type)
DROP FUNCTION _tempus_import.navteq_transport_direction(text, text, boolean);
