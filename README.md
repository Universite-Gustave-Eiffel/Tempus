Tempus
======

This is the main git repository for sources of Tempus, a framework for multimodal route planning.

It is composed of several components:

- a [core framework](https://gitlab.com/Oslandia/tempus_core) written in C++.
- a [data loader)](https://gitlab.com/Oslandia/tempus_loader) (alongside [an
  OSM loader](https://gitlab.com/Oslandia/osm2tempus) to populate its database)
- a [WPS server](https://gitlab.com/Oslandia/tempus_wps_server)
- a [WPS python client](https://gitlab.com/Oslandia/tempus_pywps)
- a [GeoJSON server](https://gitlab.com/Oslandia/tempus_geojson_server)
- a [python API](https://gitlab.com/Oslandia/pytempus)
- a [QGIS plugin](https://gitlab.com/Oslandia/tempus_qgis)
- a [set of demos](https://gitlab.com/Oslandia/tempus_demos)


Getting Started
---------------

Clone this repo with the `--recursive` option (or use `git submodule init &&
git submodule update` after cloning it), then head over to the README of each
submodule for instructions.
