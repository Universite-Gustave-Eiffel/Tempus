-- Add a metadata table with the SRID
CREATE TABLE tempus.metadata
(
   key text primary key,
   value text
);

INSERT INTO tempus.metadata VALUES ( 'srid', '2154' );
