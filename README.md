Tempus
======

This is the main git repository for sources of Tempus, a framework for multimodal route planning.

It is composed of several components:

- a [core framework](https://gitlab.com/Oslandia/tempus_core) written in C++.
- a [data loader](https://gitlab.com/Oslandia/tempus_loader) (alongside [an
  OSM loader](https://gitlab.com/Oslandia/osm2tempus) to populate its database)
- a [WPS server](https://gitlab.com/Oslandia/tempus_wps_server)
- a [WPS python client](https://gitlab.com/Oslandia/tempus_pywps)
- a [GeoJSON server](https://gitlab.com/Oslandia/tempus_geojson_server)
- a [python API](https://gitlab.com/Oslandia/pytempus)
- a [QGIS plugin](https://gitlab.com/Oslandia/tempus_qgis)
- a [set of demos](https://gitlab.com/Oslandia/tempus_demos)

Latest tags
---------

| Component  | Tag    |
|------------|--------|
| core       | [v2.6.2](https://gitlab.com/Oslandia/tempus_core/tags/v2.6.2) |
| wps server | [v1.2.1](https://gitlab.com/Oslandia/tempus_wps_server/tags/v1.2.1) |
| pytempus   | [v1.2.3](https://gitlab.com/Oslandia/pytempus/tags/v1.2.3) |
| osm2tempus | [v1.1.0](https://gitlab.com/Oslandia/osm2tempus/tags/v1.1.0) |
| loader     | [v1.2.2](https://gitlab.com/Oslandia/tempus_loader/tags/v1.2.2) |
| pywps      | [v1.0.0](https://gitlab.com/Oslandia/tempus_pywps/tags/v1.0.0) |
| qgis plugin| [v1.1.0](https://gitlab.com/Oslandia/tempus_qgis/tags/v1.1.0) |
| pg tempus  | [v1.2.1](https://gitlab.com/Oslandia/pgtempus/tags/v1.2.1) |
| json server| [v1.0.2](https://gitlab.com/Oslandia/tempus_geojson_server/tags/v1.0.2) |

Getting Started
---------------

Clone this repo with the `--recursive` option (or use `git submodule init &&
git submodule update` after cloning it), then head over to the README of each
submodule for instructions.

Windows installation
--------------------

Tempus packages are available for the [OSGeo4W distribution](https://trac.osgeo.org/osgeo4w/).

Options:
- Download the [network installer](https://github.com/Ifsttar/Tempus/releases/download/v2.6.2/setup-tempus-2.6.2.exe) that will download all the required packages
- Download the [bundled installer](https://github.com/Ifsttar/Tempus/releases/download/v2.6.2/setup-tempus-2.6.2-local.exe) that contains all the required packages in one archive.
- Use the regular [OSGeo4W network installer](http://download.osgeo.org/osgeo4w/osgeo4w-setup-x86_64.exe) and add `http://osgeo4w-oslandia.com/extra` when asked for mirrors, you should then be able to select new packages like `tempus-core` or `python3-pytempus`
