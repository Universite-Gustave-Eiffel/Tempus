@ECHO OFF
:: Get the location of postgresql
FOR /F "tokens=2* delims=	 " %%A IN ('REG QUERY "HKEY_LOCAL_MACHINE\SOFTWARE\PostgreSQL Global Development Group\PostgreSQL" /v Location') DO SET PGPATH=%%B
copy "%PGPATH%\data\pg_hba.conf" "%PGPATH%\data\pg_hba.conf.back">nul
copy data\pg_hba.conf "%PGPATH%\data\">nul
"%PGPATH%\bin\dropdb" -U postgres tempus_test_db 
"%PGPATH%\bin\createdb" -U postgres tempus_test_db
"%PGPATH%\bin\psql" -U postgres -d tempus_test_db -f data\create_extension_postgis.sql
"%PGPATH%\bin\psql" -U postgres -d tempus_test_db -f data\tempus_test_db.sql