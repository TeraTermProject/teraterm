@echo off
setlocal

:retry
echo 1. Visual Studio 15 2017
echo 2. Visual Studio 14 2015
echo 3. Visual Studio 12 2013
echo 4. Visual Studio 11 2012
echo 5. Visual Studio 10 2010
echo 6. Visual Studio 9 2008
echo 7. Visual Studio 8 2005
set /p no="select no "

echo %no%
if "%no%" == "1" set GENERATOR="Visual Studio 15 2017" & goto build_all
if "%no%" == "2" set GENERATOR="Visual Studio 14 2015" & goto build_all
if "%no%" == "3" set GENERATOR="Visual Studio 12 2013" & goto build_all
if "%no%" == "4" set GENERATOR="Visual Studio 11 2012" & goto build_all
if "%no%" == "5" set GENERATOR="Visual Studio 10 2010" & goto build_all
if "%no%" == "6" set GENERATOR="Visual Studio 9 2008" & goto build_all
if "%no%" == "7" set GENERATOR="Visual Studio 8 2005" & goto build_all
echo ? retry
goto retry

:build_all
echo cmake -DGENERATOR=%GENERATOR% -P buildall.cmake
pause
cmake -DGENERATOR=%GENERATOR% -P buildall.cmake
endlocal
pause
