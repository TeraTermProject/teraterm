setlocal
set COMPILER=VS_80
set GENERATOR=Visual Studio 8 2005
set CMAKE_COMMAND=..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe
set CMAKE_OPTION_LIBS=-DARCHITECTURE=Win32
set CMAKE_OPTION_GENERATE=
set CMAKE_OPTION_BUILD=--config Release
set BUILD_DIR=build_vs2005
set REV=9999
set DATE_TIME=20200228
set ZIP_FILE=snapshot-r%REV%-%DATE_TIME%-appveyor-vs2005.zip
set SNAPSHOT_DIR=snapshot-r%REV%-%DATE_TIME%-appveyor-vs2005

cd /d %~dp0..
call ci_scripts\build_appveyor.bat
