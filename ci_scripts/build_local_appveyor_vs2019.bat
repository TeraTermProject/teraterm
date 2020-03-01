setlocal
set COMPILER=VS_142
set GENERATOR=Visual Studio 16 2019
set CMAKE_COMMAND=cmake
set CMAKE_OPTION_LIBS=-DARCHITECTURE=Win32
set CMAKE_OPTION_GENERATE=-A Win32
set CMAKE_OPTION_BUILD=--config Release
set BUILD_DIR=build_vs2019
set REV=9999
set DATE_TIME=20200228
set ZIP_FILE=snapshot-r%REV%-%DATE_TIME%-appveyor-vs2019.zip
set SNAPSHOT_DIR=snapshot-r%REV%-%DATE_TIME%-appveyor-vs2019

cd /d %~dp0..
call ci_scripts\find_cmake.bat
call ci_scripts\build_appveyor.bat
