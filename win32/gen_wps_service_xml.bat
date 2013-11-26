@echo ^<service^> > wps_service.xml
@echo ^<id^>wps^</id^> >> wps_service.xml
@echo ^<name^>wps^</name^> >> wps_service.xml
@echo ^<description^>wps^</description^> >> wps_service.xml
@echo ^<env name=^"TEMPUS_DATA_DIRECTORY^" value=^"%CD%\data^"/^> >> wps_service.xml
@echo ^<executable^>"%CD%\bin\wps.exe"^</executable^> >> wps_service.xml
@echo ^<workingdirectory^>^"%CD%\lib^"^</workingdirectory^> >> wps_service.xml
@echo ^<arguments^>-p 9000 -l sample_pt_plugin -l sample_road_plugin -l sample_multi_plugin -d ^"host=127.0.0.1 user=postgres dbname=tempus_test_db^"^</arguments^> >> wps_service.xml
@echo ^</service^> >> wps_service.xml
