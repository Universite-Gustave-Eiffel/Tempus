call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
set PATH=%PATH%;C:\Program Files\CMake\bin;C:\Program Files\PostgreSQL\9.6\bin
mkdir build
cd build
cmake -G "NMake Makefiles" ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  -DCMAKE_INSTALL_PREFIX=c:\install ^
  "-DPOSTGRESQL_PG_CONFIG=C:\Program Files\Postgresql\9.6\bin\pg_config.exe" ^
  -DBOOST_LIBRARYDIR=C:\osgeo4w64\lib ^
  -DBOOST_INCLUDEDIR=C:\osgeo4w64\include\boost-1_63 ^
  -DBoost_COMPILER=-vc140 ^
  -DLIBXML2_INCLUDE_DIR=C:\osgeo4w64\include ^
  -DLIBXML2_LIBRARIES=C:\osgeo4w64\lib ^
  -DBUILD_WPS=OFF ^
  -DBUILD_QGIS_PLUGIN=OFF ^
  -DBUILD_OSM2TEMPUS=ON ^
  -DBUILD_DOC=OFF ^
  -DProtobuf_INCLUDE_DIR=C:\osgeo4w64\include ^
  -DProtobuf_LIBRARY_RELEASE=C:\osgeo4w64\lib\libprotobuf.lib ^
  -DProtobuf_LITE_LIBRARY_RELEASE=C:\osgeo4w64\lib\libprotobuf-lite.lib ^
  -DProtobuf_PROTOC_EXECUTABLE=c:\osgeo4w64\bin\protoc.exe ^
  ..
nmake
nmake install
