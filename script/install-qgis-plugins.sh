#!/bin/sh
base=$(dirname $0)
mkdir -p ~/.qgis2/python/plugins/ifsttarrouting
cp -r $base/../src/hmi_qgis/* ~/.qgis2/python/plugins/ifsttarrouting
mkdir -p ~/.qgis2/python/plugins/intralys
cp -r $base/../src/intralys/* ~/.qgis2/python/plugins/intralys
