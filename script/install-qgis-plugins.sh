#!/bin/sh
base=$(dirname $0)
mkdir -p ~/.qgis2/python/plugins/ifsttarrouting
cp -r $base/IfsttarRouting/* ~/.qgis2/python/plugins/ifsttarrouting
mkdir -p ~/.qgis2/python/plugins/intralys
cp -r $base/intralys/* ~/.qgis2/python/plugins/intralys
rm -f ~/.ifsttarrouting.prefs

