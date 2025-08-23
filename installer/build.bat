rem architecture for VsDevCmd.bat
if "%TARGET%" == "" (set TARGET=Win32)
if "%TARGET%" == "Win32" (set ARCHITECTURE=x86)
if "%TARGET%" == "x64"   (set ARCHITECTURE=x64)
if "%TARGET%" == "ARM64" (set ARCHITECTURE=arm64)
if "%TARGET%" == "ARM64" if "%HOST_ARCHITECTURE%" == "" (set HOST_ARCHITECTURE=amd64)

if not "%VSINSTALLDIR%" == "" goto vsinstdir

rem InnoSetup ����r���h���鎞�́A�W���Ŋ��ϐ��ɐݒ肳��Ă���
rem Visual Studio���I�������BVS2019���ߑł��Ńr���h�������ꍇ��
rem ���L goto ����L���ɂ��邱�ƁB
rem goto check_2019

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

:novs
@echo off
echo "Can't find Visual Studio"
echo.
echo InnoSetup����VS2019�Ńr���h���邽�߂ɂ́A���ϐ���ݒ肵�Ă��������B
echo.
echo ��
echo VS160COMNTOOLS=c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\
@echo on
pause
goto fail

:vsinstdir
rem Visual Studio�̃o�[�W��������
set VSCMNDIR="%VSINSTALLDIR%\Common7\Tools\"
set VSCMNDIR=%VSCMNDIR:\\=\%

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

:vsend

set BUILD=build
if "%1" == "rebuild" (set BUILD=rebuild)
pushd %~dp0

rem ���C�u�������R���p�C��
pushd ..\libs
CALL buildall.bat
if ERRORLEVEL 1 (
    echo "build.bat ���I�����܂�"
    goto fail
)
popd


rem ���r�W�������ω����Ă���� svnversion.h ���X�V����B
call ..\buildtools\svnrev\svnrev.bat


devenv /%BUILD% "Release|%TARGET%" %TERATERMSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% "Release|%TARGET%" %TTSSHSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% "Release|%TARGET%" %TTPROXYSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% "Release|%TARGET%" %TTXKANJISLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% "Release|%TARGET%" %TTPMENUSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% "Release|%TARGET%" %TTXSAMPLESLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD% "Release|%TARGET%" %CYGWINSLN%
if ERRORLEVEL 1 goto fail

rem cygterm ���R���p�C��
pushd ..\cygwin\cygterm
if "%BUILD%" == "rebuild" (
    make clean
    make cygterm+-x86_64-clean
)
make cygterm+-x86_64 -j
popd

rem msys2term
if not exist c:\msys64\usr\bin\msys-2.0.dll goto msys2term_pass
setlocal
PATH=C:\msys64\usr\bin
pushd ..\cygwin\cygterm
if "%BUILD%" == "rebuild" (
    make clean
    make msys2term-clean
)
make msys2term -j
endlocal
popd
:msys2term_pass

rem cygterm+.tar.gz
pushd ..\cygwin\cygterm
make archive
popd

rem lng �t�@�C�����쐬
call makelang.bat

popd
exit /b 0

:fail
popd
exit /b 1
