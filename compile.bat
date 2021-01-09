set "HOUDINI_DIR=C:\Program Files\Side Effects Software\Houdini 18.5.408"
set "ARNOLD_DIR=deps/Arnold-6.1.0.1-windows"
set "PATH=%HFS%\bin;%PATH%"

rem If you get an error about a missing Visual Studio, set this manually.
rem If you installed Visual Studio and it doesn't work, search for iostream in your VS installation and 
rem   set this to be the parent folder of include\iostream
IF "%MSVCDir%"=="" set "MSVCDir=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.28.29333"

"%HOUDINI_DIR%\bin\hcustom.exe" -e -i ./build src/ai_ocean_samplelayers.cpp -I %ARNOLD_DIR%/include -L "%ARNOLD_DIR%\lib" -l ai.lib -l libCVEX.lib

rem -l UI.lib -l OPZ.lib -l OP3.lib -l OP2.lib -l OP1.lib -l SIM.lib -l GEO.lib -l PRM.lib -l UT.lib -l boost_system
rem add -g to compile with debug info

PAUSE
