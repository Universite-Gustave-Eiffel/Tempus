import tempus
import sys
import argparse

def main(argv):
    parser = argparse.ArgumentParser(description='Tempus test program')
    parser.add_argument(
        '--db', '-d', type=str,
        dest='db/options',
        help='set database connection options', default='dbname=tempus_test_db')
    parser.add_argument(
        '--plugin', '-p',  type=str,
        help='set the plugin name to launch', default='sample_road_plugin')
    parser.add_argument(
        '--origin', type=int,
        help='set the origin vertex id', default=21406)
    parser.add_argument(
        '--destination', type=int,
        help='set the destination vertex id', default=21015)
    parser.add_argument(
        '--modes', nargs='*',
        help='set the allowed modes (space separated)')
    parser.add_argument(
        '--pvad', type=bool,
        help='set "private vehicule at destination"', default=True)
    parser.add_argument(
        '--depart-after', type=str,
        help='set the time constraint to depart after this date & time')
    parser.add_argument(
        '--arrive-before', type=str,
        help='set the time constraint to arrive before this date & time')
    parser.add_argument(
        '--optimize-distance', type=bool,
        help='optimize distance (instead of duration)', default=False)
    parser.add_argument(
        '--options', nargs='*',
        help='set the plugin options option:type=value (space separated)')
    parser.add_argument(
        '--repeat','-n', type=int,
        help='set the repeat count (for profiling)', default=1)
    parser.add_argument(
        '--load-from','-L', type=str,
        help='set the dump file to load')

    options = vars(parser.parse_args(argv))
    options = {k:v for k,v in vars(parser.parse_args(argv)).items() if v is not None}

    plugin_options = {'db/options': options['db/options']}
    # todo parse --options


    class Progress(tempus.ProgressionCallback):
        def __call__(self, percent, finished ):
            print ('{}...'.format(percent))

    progression = Progress()

    plugin = tempus.PluginFactory.instance().create_plugin(
        options['plugin'],
        progression,
        plugin_options)

    req = tempus.Request()
    req.origin = options['origin']
    dest = tempus.Request.Step()
    dest.location = options['destination']
    dest.private_vehicule_at_destination = options['pvad']
    tc = tempus.Request.TimeConstraint()
    tc.type = tempus.Request.TimeConstraintType.NoConstraint
    dest.constraint = tc
    req.destination = options['destination'] # dest

    req.add_allowed_mode(1)

    for i in range(0, options['repeat']):
        p = plugin.request(options)
        print req.origin
        result = p.process(req)
        for r in result:
            r = r.roadmap()
            for s in r:
                print 'distance: {}, duration: {}'.format(s.cost(tempus.CostId.CostDistance), s.cost(tempus.CostId.CostDuration))


if __name__ == "__main__":

    main(sys.argv[1:])