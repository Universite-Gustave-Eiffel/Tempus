#!/bin/sh
base=$(dirname $0)
mkdir -p ~/.qgis2/python/plugins/ifsttarrouting
cp -r $base/IfsttarRouting/* ~/.qgis2/python/plugins/ifsttarrouting
rm -f ~/.ifsttarrouting.prefs

