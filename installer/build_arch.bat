rem Copyright (C) 2007- Tera Term Project
rem All rights reserved.
rem
rem アーキテクチャごとの成果物をビルドする
setlocal


rem architecture for VsDevCmd.bat
if "%TARGET%" == "" (set TARGET=Win32)
if "%TARGET%" == "Win32" (set ARCHITECTURE=x86)
if "%TARGET%" == "x64"   (set ARCHITECTURE=x64)
if "%TARGET%" == "ARM64" (set ARCHITECTURE=arm64)
if "%TARGET%" == "ARM64" if "%HOST_ARCHITECTURE%" == "" (set HOST_ARCHITECTURE=amd64)


rem build or rebuild
set BUILD_CONF=build
if "%1" == "rebuild" (set BUILD_CONF=rebuild)


rem Change the working directory to the location of this BAT file
pushd %~dp0


rem Find Visual Studio
if not "%VSINSTALLDIR%" == "" goto vsinstdir

:check_2022
if "%VS170COMNTOOLS%" == "" goto check_2019
if not exist "%VS170COMNTOOLS%\VsDevCmd.bat" goto novs
call "%VS170COMNTOOLS%\VsDevCmd.bat" -arch=%ARCHITECTURE% -host_arch=%HOST_ARCHITECTURE%
goto vs2022

:check_2019
if "%VS160COMNTOOLS%" == "" goto novs
if not exist "%VS160COMNTOOLS%\VsDevCmd.bat" goto novs
call "%VS160COMNTOOLS%\VsDevCmd.bat" -arch=%ARCHITECTURE% -host_arch=%HOST_ARCHITECTURE%
goto vs2019

:vsinstdir
rem Check Visual Studio version
set VSCMNDIR="%VSINSTALLDIR%\Common7\Tools\"
set VSCMNDIR=%VSCMNDIR:\\=\%

if /I %VSCMNDIR% EQU "%VS180COMNTOOLS%" goto vs2026
if /I %VSCMNDIR% EQU "%VS170COMNTOOLS%" goto vs2022
if /I %VSCMNDIR% EQU "%VS160COMNTOOLS%" goto vs2019

echo Unknown Visual Studio version
goto fail

:vs2019
set TERATERMSLN=..\teraterm\ttermpro.v16.sln
set TTSSHSLN=..\ttssh2\ttssh.v16.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v16.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v16.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v16.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v16.sln
set CYGWINSLN=..\CYGWIN\cygwin.v16.sln
goto vsend

:vs2022
set TERATERMSLN=..\teraterm\ttermpro.v17.sln
set TTSSHSLN=..\ttssh2\ttssh.v17.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v17.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v17.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v17.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v17.sln
set CYGWINSLN=..\CYGWIN\cygwin.v17.sln
goto vsend

:vs2026
set TERATERMSLN=..\teraterm\ttermpro.v18.slnx
set TTSSHSLN=..\ttssh2\ttssh.v18.slnx
set TTPROXYSLN=..\TTProxy\TTProxy.v18.slnx
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v18.slnx
set TTPMENUSLN=..\ttpmenu\ttpmenu.v18.slnx
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v18.slnx
set CYGWINSLN=..\CYGWIN\cygwin.v18.slnx
goto vsend

:vsend


rem リビジョンが変化していれば svnversion.h を更新する。
call ..\buildtools\svnrev\svnrev.bat


devenv /%BUILD_CONF% "Release|%TARGET%" %TERATERMSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD_CONF% "Release|%TARGET%" %TTSSHSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD_CONF% "Release|%TARGET%" %TTPROXYSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD_CONF% "Release|%TARGET%" %TTXKANJISLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD_CONF% "Release|%TARGET%" %TTPMENUSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD_CONF% "Release|%TARGET%" %TTXSAMPLESLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD_CONF% "Release|%TARGET%" %CYGWINSLN%
if ERRORLEVEL 1 goto fail

popd
endlocal
exit /b 0


:fail
popd
endlocal
exit /b 1
