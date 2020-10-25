set "HOUDINI_DIR=C:\Program Files\Side Effects Software\Houdini 18.0.416"
set "ARNOLD_DIR=deps/Arnold-6.0.3.0-windows"
set "PATH=%HFS%\bin;%PATH%"

"%HOUDINI_DIR%\bin\hcustom.exe" -e -i ./build src/ai_ocean_samplelayers.cpp -I %ARNOLD_DIR%/include -L "%ARNOLD_DIR%\lib" -l ai.lib -l libCVEX.a

rem -l UI.lib -l OPZ.lib -l OP3.lib -l OP2.lib -l OP1.lib -l SIM.lib -l GEO.lib -l PRM.lib -l UT.lib -l boost_system
rem add -g to compile with debug info

PAUSE
