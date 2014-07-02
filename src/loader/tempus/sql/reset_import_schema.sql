/* Drop import schema and recreate it */
DROP SCHEMA IF EXISTS _tempus_import CASCADE;
CREATE SCHEMA _tempus_import;

CREATE EXTENSION IF NOT EXISTS postgis; 
