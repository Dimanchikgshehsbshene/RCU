@echo off
set ROOT=build
set PATCHES=Source/Atmosphere-Patches
copy "%PATCHES%\secmon_memory_layout.hpp" "%ROOT%\libraries\libexosphere/include/exosphere/secmon/" /Y
copy "%PATCHES%\secmon_emc_access_table_data.inc" "%ROOT%\exosphere/program/source/smc/" /Y
copy "%PATCHES%\secmon_soctherm_access_table_data.inc" "%ROOT%\exosphere/program/source/smc/" /Y
copy "%PATCHES%\secmon_define_emc_access_table.inc" "%ROOT%\exosphere/program/source/smc/" /Y
copy "%PATCHES%\secmon_define_soctherm_access_table.inc" "%ROOT%\exosphere/program/source/smc/" /Y
copy "%PATCHES%\secmon_smc_register_access.cpp" "%ROOT%\exosphere/program/source/smc/" /Y

echo Patched!
pause
