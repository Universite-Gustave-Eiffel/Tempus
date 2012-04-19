SET CLIENT_ENCODING TO UTF8;
SET STANDARD_CONFORMING_STRINGS TO ON;
BEGIN;
CREATE TABLE "public"."pointtest" (gid serial PRIMARY KEY,
"id" numeric(10,0),
"attribut1" varchar(80),
"attribut2" numeric(10,0),
"attribut3" numeric(10,0));
SELECT AddGeometryColumn('public','pointtest','the_geom','-1','POINT',2);
INSERT INTO "public"."pointtest" ("id","attribut1","attribut2","attribut3",the_geom) VALUES ('1','foobar','3','5','0101000000946B29188E3DD9BF8839CC610E73E03F');
CREATE INDEX "pointtest_the_geom_gist" ON "public"."pointtest" using gist ("the_geom" gist_geometry_ops);
COMMIT;
