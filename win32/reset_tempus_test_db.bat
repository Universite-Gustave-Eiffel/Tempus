@ECHO OFF
:: Get the location of postgresql

:: test if we are on a 64bits version
IF DEFINED ProgramW6432 goto w64
FOR /F "tokens=2* delims=	 " %%A IN ('REG QUERY "HKEY_LOCAL_MACHINE\SOFTWARE\PostgreSQL Global Development Group\PostgreSQL" /v Location') DO SET PGPATH=%%B
SET conf_file=pg_hba_w32.conf
goto next
: w64
:: test 64bits version
FOR /F "tokens=2* delims=	 " %%A IN ('REG QUERY "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\PostgreSQL Global Development Group\PostgreSQL" /v Location') DO SET PGPATH=%%B
SET conf_file=pg_hba_w64.conf
:next
IF "%PGPATH%"=="" (
ECHO "Cannot find PostgreSQL installation
GOTO end
)

:: pg_hba.conf will be overwritten to force a "trust" connection with the postgres user
copy "%PGPATH%\data\pg_hba.conf" "%PGPATH%\data\pg_hba.conf.back">nul
copy data\%conf_file% "%PGPATH%\data\pg_hba.conf">nul

:: decompress .sql.zip
SET cpath=%~dp0
cscript //nologo unzip.vbs "%cpath%data\tempus_test_db.sql.zip" "%cpath%data"

"%PGPATH%\bin\dropdb" -U postgres tempus_test_db 
"%PGPATH%\bin\createdb" -U postgres tempus_test_db
"%PGPATH%\bin\psql" -U postgres -d tempus_test_db -f data\create_extension_postgis.sql
"%PGPATH%\bin\psql" -U postgres -d tempus_test_db -f data\tempus_test_db.sql
:end
