rem Copyright (C) 2025- Tera Term Project
rem All rights reserved.
rem
rem 共通の成果物をビルドする
rem   chm         ... HTML help
rem   cygtool.dll ... Inno Setup から呼び出すので x86
rem   cygterm.exe ... Cygwin 64bit (x64) に依存しているので x64
rem   cygtoo.dll  ... MSYS2 が 64bit なので x64
rem   cygterm.+.tar.gz
rem   lang_utf8 の lng ファイルを lang_utf16le と lang に変換
rem
rem 成果物はここにまとめる
rem  - Output/build/teraterm_common
setlocal


rem TARGET for package
rem build_common.bat では Win32 を使う
set TARGET=Win32


rem architecture for VsDevCmd.bat
rem build_common.bat では x86 を使う
set ARCHITECTURE=x86


rem build or rebuild
set BUILD_CONF=build
if "%1" == "rebuild" (set BUILD_CONF=rebuild)


rem Change the working directory to the location of this BAT file
pushd %~dp0


rem Build HTML help
pushd ..\doc
CALL makechm.bat
popd


rem Find Visual Studio
if not "%VSINSTALLDIR%" == "" goto vsinstdir

:check_2022
if "%VS170COMNTOOLS%" == "" goto check_2019
if not exist "%VS170COMNTOOLS%\VsDevCmd.bat" goto novs
call "%VS170COMNTOOLS%\VsDevCmd.bat" -arch=%ARCHITECTURE%
goto vs2022

:check_2019
if "%VS160COMNTOOLS%" == "" goto novs
if not exist "%VS160COMNTOOLS%\VsDevCmd.bat" goto novs
call "%VS160COMNTOOLS%\VsDevCmd.bat" -arch=%ARCHITECTURE%
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
set CYGWINSLN=..\CYGWIN\cygwin.v16.sln
goto vsend

:vs2022
set TERATERMSLN=..\teraterm\ttermpro.v17.sln
set CYGWINSLN=..\CYGWIN\cygwin.v17.sln
goto vsend

:vs2026
set TERATERMSLN=..\teraterm\ttermpro.v18.slnx
set CYGWINSLN=..\CYGWIN\cygwin.v18.slnx
goto vsend

:vsend

rem Build cygtool.dll
devenv /%BUILD_CONF% "Release|%TARGET%" /project common_static %TERATERMSLN%
if ERRORLEVEL 1 goto fail
devenv /%BUILD_CONF% "Release|%TARGET%" /project cygtool %CYGWINSLN%
if ERRORLEVEL 1 goto fail


rem Compile cygterm.exe
pushd ..\cygwin\cygterm
if "%BUILD_CONF%" == "rebuild" (
    make clean
    make cygterm+-x86_64-clean
)
make cygterm+-x86_64 -j
popd


rem Compile msys2term.exe
if not exist c:\msys64\usr\bin\msys-2.0.dll goto msys2term_pass
setlocal
PATH=C:\msys64\usr\bin
pushd ..\cygwin\cygterm
if "%BUILD_CONF%" == "rebuild" (
    make clean
    make msys2term-clean
)
make msys2term -j
endlocal
popd
:msys2term_pass


rem Compress cygterm+.tar.gz
pushd ..\cygwin\cygterm
make archive
popd


rem Convert lng files
call makelang.bat


set dst=%~dp0Output\build\teraterm_common
echo dst=%dst%

rem ファイルを %dst% にコピーする
del /s /q %dst%\*.*
mkdir %dst%

copy /y ..\doc\ja\teratermj.chm %dst%
copy /y ..\doc\en\teraterm.chm %dst%

copy /y ..\cygwin\Release.%TARGET%\cygtool.dll %dst%

copy /y ..\cygwin\cygterm\cygterm.cfg %dst%
copy /y "..\cygwin\cygterm\cygterm+.tar.gz" %dst%
copy /y "..\cygwin\cygterm\cygterm+-x86_64\cygterm.exe" %dst%

if not exist ..\cygwin\cygterm\msys2term\msys2term.exe goto msys2term_pass
copy /y ..\cygwin\cygterm\msys2term\msys2term.exe %dst%
copy /y ..\cygwin\cygterm\msys2term.cfg %dst%
:msys2term_pass

mkdir %dst%\lang
mkdir %dst%\lang_utf16le
xcopy /s /e /y /i /exclude:archive-exclude.txt release\lang %dst%\lang
xcopy /s /e /y /i /exclude:archive-exclude.txt release\lang_utf16le %dst%\lang_utf16le
del /f %dst%\lang\en_US.lng


popd
endlocal
exit /b 0


:fail
popd
endlocal
exit /b 1
