@echo off

set TTMACRO=%~dp0\..\teraterm\Debug\ttpmacro.exe
set MACROFILE=%~dpn0.ttl

echo == test 1 ==
"%TTMACRO%" "%MACROFILE%" /vxx /ixx /v /i test1
if ERRORLEVEL 1 echo test 1 failed.

echo == test 2 ==
"%TTMACRO%" "%MACROFILE%" /v /i test2
if ERRORLEVEL 1 echo test 2 failed.

echo == test 3 ==
"%TTMACRO%" "%MACROFILE%" test3 /vxx /ixx /v /i
if ERRORLEVEL 1 echo test 3 failed.

pause
