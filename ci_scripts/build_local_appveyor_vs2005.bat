setlocal
set COMPILER=VS_80
set COMPILER_FRIENDLY=vs2005
set GENERATOR=Visual Studio 8 2005
set CMAKE_COMMAND=..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe
set CMAKE_OPTION_LIBS=-DARCHITECTURE=Win32
set CMAKE_OPTION_GENERATE=
set CMAKE_OPTION_BUILD=--config Release
set BUILD_DIR=build_%COMPILER_FRIENDLY%

cd /d %~dp0..
call ci_scripts\build_appveyor.bat
