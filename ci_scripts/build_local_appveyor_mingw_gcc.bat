setlocal
set COMPILER=mingw
set GENERATOR=Unix Makefiles
set CMAKE_COMMAND=cmake
set CMAKE_OPTION_LIBS=
set CMAKE_OPTION_GENERATE=-DCMAKE_BUILD_TYPE=Release
set CMAKE_OPTION_BUILD=
set MINGW_CC=gcc
set MINGW_CXX=g++
set BUILD_DIR=build_%COMPILER%_msys2_gcc
set REV=9999
set DATE_TIME=20200228
set ZIP_FILE=snapshot-r%REV%-%DATE_TIME%-appveyor-%COMPILER%.zip
set SNAPSHOT_DIR=snapshot-r%REV%-%DATE_TIME%-appveyor-%COMPILER%

cd /d %~dp0..
call ci_scripts\build_appveyor.bat
