@ECHO OFF
mkdir "%USERPROFILE%\.qgis2\python\plugins"
xcopy /Y /S /I python\plugins "%USERPROFILE%\.qgis2\python\plugins"
