
if not "%VSINSTALLDIR%" == "" goto vsinstdir

rem InnoSetup からビルドする時は、標準で環境変数に設定されている
rem Visual Studioが選択される。VS2019決め打ちでビルドしたい場合は
rem 下記 goto 文を有効にすること。
rem goto check_2019

if "%VS80COMNTOOLS%" == "" goto check_2008
if not exist "%VS80COMNTOOLS%\vsvars32.bat" goto check_2008
call "%VS80COMNTOOLS%\vsvars32.bat"
goto vs2005

:check_2008
if "%VS90COMNTOOLS%" == "" goto check_2010
if not exist "%VS90COMNTOOLS%\vsvars32.bat" goto check_2010
call "%VS90COMNTOOLS%\vsvars32.bat"
goto vs2008

:check_2010
if "%VS100COMNTOOLS%" == "" goto check_2012
if not exist "%VS100COMNTOOLS%\vsvars32.bat" goto check_2012
call "%VS100COMNTOOLS%\vsvars32.bat"
goto vs2010

:check_2012
if "%VS110COMNTOOLS%" == "" goto check_2013
if not exist "%VS110COMNTOOLS%\VsDevCmd.bat" goto check_2013
call "%VS110COMNTOOLS%\VsDevCmd.bat"
goto vs2012

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
@echo off
echo "Can't find Visual Studio"
echo.
echo InnoSetupからVS2019でビルドするためには、環境変数を設定してください。
echo.
echo 例
echo VS160COMNTOOLS=c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\
@echo on
pause
exit /b

:vsinstdir
rem Visual Studioのバージョン判別
set VSCMNDIR="%VSINSTALLDIR%\Common7\Tools\"
set VSCMNDIR=%VSCMNDIR:\\=\%

if /I %VSCMNDIR% EQU "%VS80COMNTOOLS%" goto vs2005
if /I %VSCMNDIR% EQU "%VS90COMNTOOLS%" goto vs2008
if /I %VSCMNDIR% EQU "%VS100COMNTOOLS%" goto vs2010
if /I %VSCMNDIR% EQU "%VS110COMNTOOLS%" goto vs2012
if /I %VSCMNDIR% EQU "%VS120COMNTOOLS%" goto vs2013
if /I %VSCMNDIR% EQU "%VS140COMNTOOLS%" goto vs2015
if /I %VSCMNDIR% EQU "%VS150COMNTOOLS%" goto vs2017
if /I %VSCMNDIR% EQU "%VS160COMNTOOLS%" goto vs2019

echo Unknown Visual Studio version
exit /b

:vs2005
set TERATERMSLN=..\teraterm\ttermpro.sln
set TTSSHSLN=..\ttssh2\ttssh.sln
set TTPROXYSLN=..\TTProxy\TTProxy.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.sln
goto vsend

:vs2008
set TERATERMSLN=..\teraterm\ttermpro.v9.sln
set TTSSHSLN=..\ttssh2\ttssh.v9.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v9.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v9.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v9.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v9.sln
goto vsend

:vs2010
set TERATERMSLN=..\teraterm\ttermpro.v10.sln
set TTSSHSLN=..\ttssh2\ttssh.v10.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v10.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v10.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v10.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v10.sln
goto vsend

:vs2012
set TERATERMSLN=..\teraterm\ttermpro.v11.sln
set TTSSHSLN=..\ttssh2\ttssh.v11.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v11.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v11.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v11.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v11.sln
goto vsend

:vs2013
set TERATERMSLN=..\teraterm\ttermpro.v12.sln
set TTSSHSLN=..\ttssh2\ttssh.v12.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v12.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v12.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v12.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v12.sln
goto vsend

:vs2015
set TERATERMSLN=..\teraterm\ttermpro.v14.sln
set TTSSHSLN=..\ttssh2\ttssh.v14.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v14.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v14.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v14.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v14.sln
goto vsend

:vs2017
set TERATERMSLN=..\teraterm\ttermpro.v15.sln
set TTSSHSLN=..\ttssh2\ttssh.v15.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v15.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v15.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v15.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v15.sln
goto vsend

:vs2019
set TERATERMSLN=..\teraterm\ttermpro.v16.sln
set TTSSHSLN=..\ttssh2\ttssh.v16.sln
set TTPROXYSLN=..\TTProxy\TTProxy.v16.sln
set TTXKANJISLN=..\TTXKanjiMenu\ttxkanjimenu.v16.sln
set TTPMENUSLN=..\ttpmenu\ttpmenu.v16.sln
set TTXSAMPLESLN=..\TTXSamples\TTXSamples.v16.sln
goto vsend

:vsend

set BUILD=build
if "%1" == "rebuild" (set BUILD=rebuild)
pushd %~dp0

rem ライブラリをコンパイル
pushd ..\libs
CALL buildall.bat
popd

if "%BUILD%" == "rebuild" goto build

rem "rebuild"を指定しない場合、SVNリビジョンを更新する。
if exist ..\teraterm\release\svnrev.exe goto svnrev
devenv /build release %TERATERMSLN% /project svnrev /projectconfig release

:svnrev
..\teraterm\release\svnrev.exe ..\libs\svn\bin\svnversion.exe .. ..\teraterm\ttpdlg\svnversion.h

:build
devenv /%BUILD% release %TERATERMSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% release %TTSSHSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% release %TTPROXYSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% release %TTXKANJISLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% release %TTPMENUSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% release %TTXSAMPLESLN%
if ERRORLEVEL 1 goto fail

rem cygterm をコンパイル
pushd ..\cygterm
if "%BUILD%" == "rebuild" make clean
make
popd

rem cygtool をコンパイル
pushd cygtool
nmake -f cygtool.mak
popd

rem lng ファイルを作成
call makelang.bat

popd
exit /b 0

:fail
popd
exit /b 1
