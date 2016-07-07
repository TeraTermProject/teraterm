@echo off

set TTMACRO=%~dp0\..\teraterm\Debug\ttpmacro.exe
set MACROFILE=%~dpn0.ttl

echo == test 1 ==
"%TTMACRO%" "%MACROFILE%" /vxx /ixx /V /i test1 "param 7" "" param9 10 eleven
if ERRORLEVEL 1 echo test 1 failed.

pause
