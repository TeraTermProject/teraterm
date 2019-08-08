pushd libressl

SET LIBRESSL_BUILD=FALSE

IF NOT EXIST crypto\Debug\crypto.lib SET LIBRESSL_BUILD=TRUE
IF NOT EXIST crypto\Release\crypto.lib SET LIBRESSL_BUILD=TRUE

IF %LIBRESSL_BUILD%==FALSE GOTO build_end


if not "%VSINSTALLDIR%" == "" goto vsinstdir

:check_2013
if "%VS120COMNTOOLS%" == "" goto check_2015
if not exist "%VS120COMNTOOLS%\VsDevCmd.bat" goto check_2015
call "%VS120COMNTOOLS%\VsDevCmd.bat"
goto vs2013

:check_2015
if "%VS140COMNTOOLS%" == "" goto check_2017
if not exist "%VS140COMNTOOLS%\VsDevCmd.bat" goto check_2017
call "%VS140COMNTOOLS%\VsDevCmd.bat"
goto vs2015

:check_2017
if "%VS150COMNTOOLS%" == "" goto check_2019
if not exist "%VS150COMNTOOLS%\VsDevCmd.bat" goto check_2019
call "%VS150COMNTOOLS%\VsDevCmd.bat"
goto vs2017

:check_2019
if "%VS160COMNTOOLS%" == "" goto novs
if not exist "%VS160COMNTOOLS%\VsDevCmd.bat" goto novs
call "%VS160COMNTOOLS%\VsDevCmd.bat"
goto vs2019

:novs
echo "Can't find Visual Studio"
exit /b

:vsinstdir
rem Visual StudioÇÃÉoÅ[ÉWÉáÉìîªï 
set VSCMNDIR="%VSINSTALLDIR%\Common7\Tools\"
set VSCMNDIR=%VSCMNDIR:\\=\%

if /I %VSCMNDIR% EQU "%VS120COMNTOOLS%" goto vs2013
if /I %VSCMNDIR% EQU "%VS140COMNTOOLS%" goto vs2015
if /I %VSCMNDIR% EQU "%VS150COMNTOOLS%" goto vs2017
if /I %VSCMNDIR% EQU "%VS160COMNTOOLS%" goto vs2019

echo Unknown Visual Studio version
exit /b

:vs2013
set CMAKE_GENERATOR=Visual Studio 12 2013
goto vsend

:vs2015
set CMAKE_GENERATOR=Visual Studio 14 2015
goto vsend

:vs2017
set CMAKE_GENERATOR=Visual Studio 15 2016
goto vsend

:vs2019
set CMAKE_GENERATOR=Visual Studio 16 2019
goto vsend

:vsend


cmake -G "%CMAKE_GENERATOR%" -A Win32
perl -pi.bak -e "s/MD/MT/g" CMakeCache.txt
cmake -G "%CMAKE_GENERATOR%" -A Win32

:build_debug
IF EXIST crypto\Debug\crypto.lib GOTO build_release
devenv /build Debug LibreSSL.sln /project crypto /projectconfig Debug

:build_release
IF EXIST crypto\Release\crypto.lib GOTO build_end
devenv /build Release LibreSSL.sln /project crypto /projectconfig Release

:build_end
popd
