call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
set PATH=%PATH%;C:\Program Files\CMake\bin
cd build
cmake -G "NMake Makefiles" -D BOOST_LIBRARYDIR=C:\osgeo4w64\lib -D BOOST_INCLUDEDIR=C:\osgeo4w64\include\boost-1_56 -D Boost_COMPILER=-vc110 -D LIBXML2_INCLUDE_DIR=C:\osgeo4w64\include -D LIBXML2_LIBRARIES=C:\osgeo4w64\lib -D BUILD_WPS=OFF -D BUILD_QGIS_PLUGIN=OFF -D BUILD_OSM2TEMPUS=OFF -D BUILD_DOC=OFF ..
nmake