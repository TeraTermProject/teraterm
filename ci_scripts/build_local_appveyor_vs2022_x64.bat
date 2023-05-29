setlocal
set PATH=
set COMPILER=VS_143_x64
set COMPILER_FRIENDLY=vs2022_x64
set GENERATOR=Visual Studio 17 2022
set CMAKE_COMMAND=cmake
set CMAKE_OPTION_LIBS=-DARCHITECTURE=64
set CMAKE_OPTION_GENERATE=-A x64
set CMAKE_OPTION_BUILD=--config Release
set BUILD_DIR=build_%COMPILER_FRIENDLY%

cd /d %~dp0..
call ci_scripts\find_cmake.bat
call ci_scripts\build_appveyor.bat
pause
