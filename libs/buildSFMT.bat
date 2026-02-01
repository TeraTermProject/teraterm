rem Build SFMT
setlocal

pushd SFMT


SET filename=SFMT_version_for_teraterm.h


rem architecture for VsDevCmd.bat
if "%TARGET%" == "" (set TARGET=Win32)
if "%TARGET%" == "Win32" (set ARCHITECTURE=x86)
if "%TARGET%" == "x64"   (set ARCHITECTURE=x64)
if "%TARGET%" == "ARM64" (set ARCHITECTURE=arm64)
if "%TARGET%" == "ARM64" if "%HOST_ARCHITECTURE%" == "" (set HOST_ARCHITECTURE=amd64)


rem Find Visual Studio
if not "%VSINSTALLDIR%" == "" goto vsinstdir

:check_2019
if "%VS160COMNTOOLS%" == "" goto check_2022
if not exist "%VS160COMNTOOLS%\VsDevCmd.bat" goto check_2022
call "%VS160COMNTOOLS%\VsDevCmd.bat" -arch=%ARCHITECTURE% -host_arch=%HOST_ARCHITECTURE%
goto generate

:check_2022
if "%VS170COMNTOOLS%" == "" goto novs
if not exist "%VS170COMNTOOLS%\VsDevCmd.bat" goto novs
call "%VS170COMNTOOLS%\VsDevCmd.bat" -arch=%ARCHITECTURE% -host_arch=%HOST_ARCHITECTURE%
goto generate

:novs
echo "Can't find Visual Studio"
goto fail

:vsinstdir
rem Check Visual Studio version
set VSCMNDIR="%VSINSTALLDIR%\Common7\Tools\"
set VSCMNDIR=%VSCMNDIR:\\=\%

if /I %VSCMNDIR% EQU "%VS160COMNTOOLS%" goto generate
if /I %VSCMNDIR% EQU "%VS170COMNTOOLS%" goto generate
if /I %VSCMNDIR% EQU "%VS180COMNTOOLS%" goto generate

echo Unknown Visual Studio version
goto fail


:generate

rem Generate Makefile
SET SFMT_MAKEFILE=Makefile.msc.%TARGET%.debug
SET SFMT_OUTDIR=build\%TARGET%\Debug

if exist "%SFMT_MAKEFILE%" goto end_debug_gen
echo CFLAGS = /MTd /Od /nologo> %SFMT_MAKEFILE%
echo;>> %SFMT_MAKEFILE%
echo %SFMT_OUTDIR%\SFMTd.lib: SFMT.c>> %SFMT_MAKEFILE%
echo 	cl $(CFLAGS) /Fo%SFMT_OUTDIR%\SFMT.obj /c SFMT.c>> %SFMT_MAKEFILE%
echo 	lib /out:%SFMT_OUTDIR%\SFMTd.lib %SFMT_OUTDIR%\SFMT.obj>> %SFMT_MAKEFILE%

:end_debug_gen

IF NOT EXIST %SFMT_OUTDIR% MKDIR %SFMT_OUTDIR%
nmake /f Makefile.msc.%TARGET%.debug


SET SFMT_MAKEFILE=Makefile.msc.%TARGET%.release
SET SFMT_OUTDIR=build\%TARGET%\Release

if exist "%SFMT_MAKEFILE%" goto end_release_gen
echo CFLAGS = /MT /O2 /nologo> %SFMT_MAKEFILE%
echo;>> %SFMT_MAKEFILE%
echo %SFMT_OUTDIR%\SFMT.lib: SFMT.c>> %SFMT_MAKEFILE%
echo 	cl $(CFLAGS) /Fo%SFMT_OUTDIR%\SFMT.obj /c SFMT.c>> %SFMT_MAKEFILE%
echo 	lib /out:%SFMT_OUTDIR%\SFMT.lib %SFMT_OUTDIR%\SFMT.obj>> %SFMT_MAKEFILE%

:end_release_gen

IF NOT EXIST %SFMT_OUTDIR% MKDIR %SFMT_OUTDIR%
nmake /f Makefile.msc.%TARGET%.release


rem Create version file
IF EXIST %filename% (GOTO FILE_TRUE) ELSE GOTO FILE_FALSE
:FILE_TRUE
ECHO "Found file: %filename%"
GOTO END

:FILE_FALSE
ECHO "%filename% file was not fould. Generating it."
echo #define SFMT_VERSION "Unknown"> %filename%
GOTO END

:END
popd
endlocal
exit /b 0


:fail
popd
echo "buildSFMT.bat failed"
@echo on
endlocal
exit /b 1
