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
| core       | [v2.4.1](https://gitlab.com/Oslandia/tempus_core/tags/v2.4.1) |
| wps server | [v1.1.0](https://gitlab.com/Oslandia/tempus_wps_server/tags/v1.1.0) |
| pytempus   | [v1.1.0](https://gitlab.com/Oslandia/pytempus/tags/v1.1.0) |
| osm2tempus | [v1.1.0](https://gitlab.com/Oslandia/osm2tempus/tags/v1.1.0) |
| loader     | [v1.1.1](https://gitlab.com/Oslandia/tempus_loader/tags/v1.1.1) |
| pywps      | [v1.0.0](https://gitlab.com/Oslandia/tempus_pywps/tags/v1.0.0) |
| qgis plugin| [v1.0.1](https://gitlab.com/Oslandia/tempus_qgis/tags/v1.0.1) |
| pg tempus  | [v1.0.1](https://gitlab.com/Oslandia/pgtempus/tags/v1.0.1) |
| json server| [v1.0.2](https://gitlab.com/Oslandia/tempus_geojson_server/tags/v1.0.2) |

Getting Started
---------------

Clone this repo with the `--recursive` option (or use `git submodule init &&
git submodule update` after cloning it), then head over to the README of each
submodule for instructions.
