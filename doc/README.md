![Image](images/qgis_5.png?raw=true)

TempusV2 is a C++ framework which offers generic graph manipulation abilities in order to develop multimodal path planning requests. It features :

* a C++ core based on [Boost::graph](http://www.boost.org/doc/libs/release/libs/graph/) for multimodal graph manipulations
* a (dynamically loadable) plugin architecture for routing algorithms
* exposure of its API as a webservice through a FastCGI WPS server that is able to respond to simultaneous requests
* a client for [QGIS](http://www.qgis.org)
* a loader script that is able to load road network data from Navteq, TomTom and OpenStreetMap, public transport data from GTFS and point of interests from CSV files
* a Python request API

TempusV2 is distributed under the terms of the GNU LGPLv2+

[Documentation](Documentation.md)

[Installation](Installation.md)
