#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Tempus data loader

import argparse
import provider
import sys


def import_tomtom(args, shape_options):
    """Load Tomtom (Multinet) data into a PostGIS database."""
    Importer = {
        '1409': provider.MultinetImporter1409,
        None: provider.MultinetImporter
    }[args.model_version]
    shape_options['I'] = False
    subs = { 'native_srid' : args.native_srid }
    mni = Importer(args.source, args.speed_profile, args.prefix, args.dbstring, args.logfile, shape_options, not args.noclean, subs)
    return mni.load()


def import_pt(args):
    """Load Public Transportation (GTFS) data into a PostGIS database."""
    subs={}
    subs['native_srid'] = args.native_srid
    if args.pt_network is None:
        sys.stderr.write("A PT network name must be supplied. Use --pt-network")
        sys.exit(1)
    subs['network'] = args.pt_network
    subs['create_road_nodes'] = args.pt_create_roads
    gtfsi = provider.GTFSImporter(args.source, args.dbstring, args.logfile, args.encoding, args.copymode, not args.noclean, subs)
    return gtfsi.load()


def import_navteq(args, shape_options):
    """Load Navteq (Navstreets) data into a PostGIS database."""
    subs = { 'native_srid' : args.native_srid }
    ntqi = provider.NavstreetsImporter(args.source, args.prefix, args.dbstring, args.logfile, shape_options, not args.noclean, subs)
    return ntqi.load()


def import_route120(args, shape_options):
    """Load IGN (Route120) data into a PostGIS database."""
    subs = { 'native_srid' : args.native_srid }
    igni = provider.IGNRoute120Importer(args.source, args.prefix, args.dbstring, args.logfile, shape_options, not args.noclean, subs)
    return igni.load()


def import_route500(args, shape_options):
    """Load IGN (Route500) data into a PostGIS database."""
    subs = { 'native_srid' : args.native_srid }
    igni = provider.IGNRoute500Importer(args.source, args.prefix, args.dbstring, args.logfile, shape_options, not args.noclean, subs)
    return igni.load()


def import_osm(args, shape_options):
    """Load OpenStreetMap (as shapefile) data into a PostGIS database."""
    subs = { 'native_srid' : args.native_srid }
    osmi = provider.OSMImporter(args.source, args.dbstring, args.logfile)
    return osmi.load()


def import_poi(args, shape_options):
    """Load a point shapefile into a PostGIS database."""
    try:
        poi_type = int(args.poi_type)
        if poi_type not in range(1, 6):
            raise ValueError
    except ValueError:
        print "Wrong poi type. Assuming User type (5). Correct values are in range 1-5."
        poi_type = 5
    if args.poi_service_name is None:
        sys.stderr.write("The service name of the POI must be specified with --poi-service-name !")
        sys.exit(1)
    subs = {}
    subs["service_name"] = args.poi_service_name
    subs["name"] = args.poi_name_field
    subs["filter"] = args.poi_filter
    subs['native_srid'] = args.native_srid
    poii = provider.POIImporter(args.source, args.prefix, args.dbstring, args.logfile, shape_options, not args.noclean, poi_type, subs)
    return poii.load()


def reset_db(args):
    subs = {'native_srid' : args.native_srid}
    r = provider.ResetImporter(source='', dbstring=args.dbstring, logfile=args.logfile, options={}, doclean=False, subs=subs)
    return r.load()

def main():
    shape_options = {}
    parser = argparse.ArgumentParser(description='Tempus data loader')
    parser.add_argument(
        '-s', '--source',
        required=False,
        nargs='+',
        help='The source directory/file to load data from')
    parser.add_argument(
        '-sp', '--speed_profile',
        required=False,
        nargs='+',
        help='The source directory/file to load speed profile data from')
    parser.add_argument(
        '-S', '--srid',
        required=False,
        help="Set the SRID for geometries. Defaults to 4326 (lat/lon).")
    parser.add_argument(
        '-N', '--native-srid',
        required=False,
        help="Set the SRID for the tempus db. Defaults to 4326. To be used when creating/reseting the base",
        default=4326)
    parser.add_argument(
        '-R', '--reset',
        required=False,
        help='Reset the database before importing',
        action='store_true', dest='doreset', default=False)
    parser.add_argument(
        '-t', '--type',
        required=False,
        help='The data type (tomtom, navteq, osm, gtfs, poi, route120, route500)')
    parser.add_argument(
        '-m', '--model-version',
        required=False, default=None, dest='model_version',
        help='The data model version (tomtom version)')
    parser.add_argument(
        '-d', '--dbstring',
        required=False,
        help='The database connection string')
    parser.add_argument(
        '-p', '--prefix',
        required=False,
        help='Prefix for shapefile names', default="")
    parser.add_argument(
        '-l', '--logfile',
        required=False,
        help='Log file for loading and SQL output')
    parser.add_argument(
        '-i', '--insert',
        required=False, action='store_false', dest='copymode', default=True,
        help='Use insert for SQL mode (default to COPY)')
    parser.add_argument(
        '-W', '--encoding',
        required=False,
        help="Specify the character encoding of Shape's attribute column.")
    parser.add_argument(
        '-n', '--noclean',
        required=False, action='store_true', default=False,
        help="Do not clean temporary SQL file after loading.")
    parser.add_argument(
        '-y', '--poi-type',
        required=False, default=5,
        help="Poi type (1: Car park, 2: shared car, 3: Cycle, 4:Shared cycle, 5:user)")
    parser.add_argument('--poi-name-field', required=False, default="pname", help="Name of the field containing the name of each POI (default 'pname')"),
    parser.add_argument('--poi-service-name', required=False, help="Name of the POI service imported (required for POI import)"),
    parser.add_argument('--poi-filter', required=False, default="true", help="WHERE clause of the POI import (default 'true', i.e. no filter)"),

    parser.add_argument('--pt-network', required=False, help="Name of the public transport to import (required for PT import)"),
    parser.add_argument('--pt-create-roads', required=False, default=False, action="store_true",
                        help="If 'true', this will create road nodes and sections for PT stops that are too far from the road network (default 'false')")

    args = parser.parse_args()

    if args.type in ('tomtom', 'navteq', 'poi'):
        if not args.srid:
            sys.stderr.write("SRID needed for %s data type. Assuming EPSG:4326.\n" % args.type)
            shape_options['s'] = 4326
        else:
            shape_options['s'] = args.srid
    if args.type in ['route120', 'route500']:
        shape_options['s'] = 2154

    if args.copymode:
        shape_options['D'] = True
    if args.encoding:
        shape_options['W'] = args.encoding
    else:
        args.encoding = 'UTF8'
    # default shp2pgsql options
    shape_options['I'] = True
    shape_options['g'] = 'geom'
    shape_options['S'] = True

    if args.type is None:
        parser.print_help()
        sys.exit(1)

    # first reset if needed
    if args.doreset:
        reset_db(args)
    else:
        # retrieve srid from database
        native_srid = int(provider.dbtools.exec_sql(args.dbstring, "select value from tempus.metadata where key='srid'"))
        print "Got %d as native SRID from the DB" % native_srid
        args.native_srid = native_srid
        
    if args.type is not None and args.source is not None:
        r = None
        if args.type == 'tomtom':
            r = import_tomtom(args, shape_options)
        elif args.type == 'osm':
            args.source = args.source[0]
            r = import_osm(args, shape_options)
        elif args.type == 'navteq':
            r = import_navteq(args, shape_options)
        elif args.type == 'route120':
            r = import_route120(args, shape_options)
        elif args.type == 'route500':
            r = import_route500(args, shape_options)
        elif args.type == 'gtfs':
            args.source = args.source[0]
            r = import_pt(args)
        elif args.type == 'poi':
            r = import_poi(args, shape_options)

        if not r:
            print "Error during import !"
            sys.exit(1)
    elif not args.doreset:
        sys.stderr.write("Source and type needed !")
        sys.exit(1)

    sys.exit(0)

if __name__ == '__main__':
    main()
