@ECHO OFF
SET wd=%~dp0
mkdir "%USERPROFILE%\.qgis2\python\plugins"
xcopy /Y /S /I "%wd%\python\plugins" "%USERPROFILE%\.qgis2\python\plugins"
if exist "%USERPROFILE%\.ifsttarrouting.prefs" del "%USERPROFILE%\.ifsttarrouting.prefs"
