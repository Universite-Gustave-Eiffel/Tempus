call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
set PATH=%PATH%;C:\Program Files\CMake\bin;C:\Program Files\PostgreSQL\9.6\bin
mkdir build
cd build
cmake -G "NMake Makefiles" -D BOOST_LIBRARYDIR=C:\local\boost_1_62_0\lib64-msvc-14.0 -D BOOST_INCLUDEDIR=C:\local\boost_1_62_0 -D Boost_COMPILER=-vc140 -D LIBXML2_INCLUDE_DIR=C:\osgeo4w64\include -D LIBXML2_LIBRARIES=C:\osgeo4w64\lib -D BUILD_WPS=OFF -D BUILD_QGIS_PLUGIN=OFF -D BUILD_OSM2TEMPUS=ON -D BUILD_DOC=OFF -DPROTOBUF_INCLUDE_DIR=C:\libs\include -DPROTOBUF_LIBRARY=C:\libs\libs\libprotobuf.lib -DPROTOBUF_LITE_LIBRARY=C:\libs\lib\libprotobuf-lite.lib -DPROTOBUF_PROTOC_EXECUTABLE=c:\libs\bin\protoc.exe ..
nmake
