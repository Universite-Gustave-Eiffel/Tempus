set OSGEO4W_ROOT=C:\osgeo4w64
call c:\osgeo4w64\bin\py3_env.bat
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
:: add python3.dll to the path
set PATH=%PATH%;c:\osgeo4w64\bin
cd python\pytempus
python setup.py bdist_wheel ^
  -I c:\osgeo4w64\include\boost-1_63 ^
  -L c:\osgeo4w64\lib ^
  -I c:\install\include ^
  -L c:\install\lib
python setup.py install
