@echo off
setlocal
set CUR=%~dp0
cd /d %CUR%

echo =======
echo 1. vs2022 win32
echo 2. vs2022 x64
echo b. vs2022 arm64
echo 3. vs2019 win32
echo 4. vs2019 x64
echo 5. mingw gcc win32
echo 6. mingw gcc x64
echo 7. mingw clang win32
echo 8. mingw clang x64
echo 9. exit
rem echo a. vs2005

if "%1" == "" (
    set /p no="select no "
) else (
    set no=%1
)
echo %no%

if not "%no%" == "1" goto pass_1
    set COMPILER=VS_143
    set COMPILER_FRIENDLY=vs2022
    set GENERATOR=Visual Studio 17 2022
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=win32
    set CMAKE_OPTION_GENERATE=-A Win32
    set CMAKE_OPTION_BUILD=--config Release
    set BUILD_DIR=build_%COMPILER_FRIENDLY%_win32
    call :build
:pass_1
if not "%no%" == "2" goto pass_2
    set COMPILER=VS_143_x64
    set COMPILER_FRIENDLY=vs2022_x64
    set GENERATOR=Visual Studio 17 2022
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=x64
    set CMAKE_OPTION_GENERATE=-A x64
    set CMAKE_OPTION_BUILD=--config Release
    set BUILD_DIR=build_%COMPILER_FRIENDLY%
    call :build
:pass_2
if not "%no%" == "b" goto pass_a
    set COMPILER=VS_143_arm64
    set COMPILER_FRIENDLY=vs2022_arm64
    set GENERATOR=Visual Studio 17 2022
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=arm64
    set CMAKE_OPTION_GENERATE=-A arm64
    set CMAKE_OPTION_BUILD=--config Release
    set BUILD_DIR=build_%COMPILER_FRIENDLY%
    call :build
:pass_a
if not "%no%" == "3" goto pass_3
    set COMPILER=VS_142
    set COMPILER_FRIENDLY=vs2019
    set GENERATOR=Visual Studio 16 2019
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=win32
    set CMAKE_OPTION_GENERATE=-A Win32
    set CMAKE_OPTION_BUILD=--config Release
    set BUILD_DIR=build_%COMPILER_FRIENDLY%
    call :build
:pass_3
if not "%no%" == "4" goto pass_4
    set COMPILER=VS_142_x64
    set COMPILER_FRIENDLY=vs2019_x64
    set GENERATOR=Visual Studio 16 2019
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=x64
    set CMAKE_OPTION_GENERATE=-A x64
    set CMAKE_OPTION_BUILD=--config Release
    set BUILD_DIR=build_%COMPILER_FRIENDLY%
    call :build
:pass_4
if not "%no%" == "a" goto pass_a
    set COMPILER=VS_80
    set COMPILER_FRIENDLY=vs2005
    set GENERATOR=Visual Studio 8 2005
    set CMAKE_COMMAND=..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=win32
    set CMAKE_OPTION_GENERATE=
    set CMAKE_OPTION_BUILD=--config Release
    set BUILD_DIR=build_%COMPILER_FRIENDLY%
:pass_a
if not "%no%" == "5" goto pass_5
    set COMPILER=mingw
    set COMPILER_FRIENDLY=mingw_gcc
    set GENERATOR=Unix Makefiles
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=i686
    set CMAKE_OPTION_GENERATE=-DCMAKE_BUILD_TYPE=Release
    set CMAKE_OPTION_BUILD=
    set MINGW_CC=gcc
    set MINGW_CXX=g++
    set BUILD_DIR=build_%COMPILER_FRIENDLY%_msys2
    call :build
:pass_5
if not "%no%" == "6" goto pass_6
    set COMPILER=mingw_x64
    set COMPILER_FRIENDLY=mingw_x64_gcc
    set GENERATOR=Unix Makefiles
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=x86_64
    set CMAKE_OPTION_GENERATE=-DCMAKE_BUILD_TYPE=Release
    set CMAKE_OPTION_BUILD=
    set MINGW_CC=gcc
    set MINGW_CXX=g++
    set BUILD_DIR=build_%COMPILER_FRIENDLY%_msys2
    call :build
:pass_6
if not "%no%" == "7" goto pass_7
    set COMPILER=mingw
    set COMPILER_FRIENDLY=mingw_clang
    set GENERATOR=Unix Makefiles
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=i686
    set CMAKE_OPTION_GENERATE=-DCMAKE_BUILD_TYPE=Release
    set CMAKE_OPTION_BUILD=
    set MINGW_CC=clang
    set MINGW_CXX=clang++
    set BUILD_DIR=build_%COMPILER_FRIENDLY%_msys2
    call :build
:pass_7
if not "%no%" == "8" goto pass_8
    set COMPILER=mingw_x64
    set COMPILER_FRIENDLY=mingw_x64_clang
    set GENERATOR=Unix Makefiles
    set CMAKE_COMMAND=cmake
    set CMAKE_OPTION_LIBS=-DARCHITECTURE=x86_64
    set CMAKE_OPTION_GENERATE=-DCMAKE_BUILD_TYPE=Release
    set CMAKE_OPTION_BUILD=
    set MINGW_CC=clang
    set MINGW_CXX=clang++
    set BUILD_DIR=build_%COMPILER_FRIENDLY%_msys2
    call :build
:pass_8

if not "%NOPAUSE%" == "1" pause
exit /b 0

rem ####################
:build
cd /d %~dp0..
call ci_scripts\find_cmake.bat
call ci_scripts\build_appveyor.bat
exit /b 0
