@echo off
rem Set up environment for Windows 95 MinGW build
rem   call this batch file in a command prompt, then run cmake
rem   - add buildtools/mingw32 gcc to the top of PATH
rem   - add MSYS2 make, cmake to PATH
rem   - set MSYSTEM=MINGW32
rem     (libs/ build scripts use mingw.toolchain.cmake when MSYSTEM is defined)
rem   see install_mingw32.md
rem
rem   errorlevel
rem     0  ok
rem     1  buildtools/mingw32 is not installed (run install_mingw32.bat)
rem     2  MSYS2 cmake is not installed
cd %~dp0

if "%CMAKE_COMMAND%" == "" (
   call ..\ci_scripts\find_cmake.bat
)

set MINGW_W95_BIN=%~dp0mingw32\bin
if not exist "%MINGW_W95_BIN%\i686-w64-mingw32-gcc.exe" (
    echo not found %MINGW_W95_BIN%\i686-w64-mingw32-gcc.exe
    echo run install_mingw32.bat
    exit /b 1
)

rem MSYS2
if exist "%~dp0msys64\usr\bin\make.exe" (
    set MSYS2_ROOT=%~dp0msys64
) else (
    set MSYS2_ROOT=C:\msys64
)
rem if not exist "%MSYS2_ROOT%\mingw32\bin\cmake.exe" (
rem     echo not found %MSYS2_ROOT%\mingw32\bin\cmake.exe
rem     echo install mingw-w64-i686-cmake
rem     exit /b 2
rem )

set PATH=%MINGW_W95_BIN%;%MSYS2_ROOT%\usr\bin;%MSYS2_ROOT%\mingw32\bin;%PATH%
set MSYSTEM=MINGW32
rem set CMAKE_COMMAND=%MSYS2_ROOT%\mingw32\bin\cmake.exe

echo MINGW_W95_BIN=%MINGW_W95_BIN%
echo MSYS2_ROOT=%MSYS2_ROOT%
echo CMAKE_COMMAND=%CMAKE_COMMAND%
"%MINGW_W95_BIN%\gcc.exe" --version | findstr gcc
exit /b 0
