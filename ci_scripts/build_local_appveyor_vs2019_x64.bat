setlocal
set COMPILER=VS_142_x64
set COMPILER_FRIENDLY=vs2019_x64
set GENERATOR=Visual Studio 16 2019
set CMAKE_COMMAND=cmake
set CMAKE_OPTION_LIBS=-DARCHITECTURE=x64
set CMAKE_OPTION_GENERATE=-A x64
set CMAKE_OPTION_BUILD=--config Release
set BUILD_DIR=build_%COMPILER_FRIENDLY%

cd /d %~dp0..
call ci_scripts\find_cmake.bat
call ci_scripts\build_appveyor.bat
