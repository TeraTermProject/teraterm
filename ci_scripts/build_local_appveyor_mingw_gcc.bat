setlocal
set COMPILER=mingw
set COMPILER_FRIENDLY=mingw_gcc
set GENERATOR=Unix Makefiles
set CMAKE_COMMAND=cmake
set CMAKE_OPTION_LIBS=
set CMAKE_OPTION_GENERATE=-DCMAKE_BUILD_TYPE=Release
set CMAKE_OPTION_BUILD=
set MINGW_CC=gcc
set MINGW_CXX=g++
set BUILD_DIR=build_%COMPILER_FRIENDLY%_msys2

cd /d %~dp0..
call ci_scripts\build_appveyor.bat
