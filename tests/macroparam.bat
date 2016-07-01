@echo off

set TTMACRO=%~dp0\..\teraterm\Debug\ttpmacro.exe
set MACROFILE=%~dpn0.ttl

echo == test 1 ==
"%TTMACRO%" "%MACROFILE%" /vxx /ixx /V /i test1
if ERRORLEVEL 1 echo test 1 failed.

echo == test 2 ==
"%TTMACRO%" /V /i "%MACROFILE%" /v /I test2
if ERRORLEVEL 1 echo test 2 failed.

echo == test 3 ==
"%TTMACRO%" /I "%MACROFILE%" test3 /Vxx /ixx /V /i
if ERRORLEVEL 1 echo test 3 failed.

echo == test 4 ==
"%TTMACRO%" /i "%MACROFILE%" test4 /V /Vxx /ixx
if ERRORLEVEL 1 echo test 4 failed.

rem ; ttpmacro.exe macrofile /vxx /ixx /V /i test1
rem ; ttpmacro.exe /V /i macrofile /v /I test2
rem ; ttpmacro.exe /I macrofile test3 /Vxx /ixx /V /i
rem ; ttpmacro.exe /i macrofile test4 /V /Vxx /ixx

pause
