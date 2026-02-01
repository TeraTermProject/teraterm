@echo off
setlocal
chcp 65001
cd /d %~dp0

if NOT "%CMAKE_COMMAND%" == "" goto pass_set_cmake
call ..\ci_scripts\find_cmake.bat
set OPT=
:pass_set_cmake

:retry_vs
rem echo f. Visual Studio 18 2026 Win32
rem echo g. Visual Studio 18 2026 x64
rem echo h. Visual Studio 18 2026 arm64
echo 1. Visual Studio 17 2022 Win32
echo 2. Visual Studio 17 2022 x64
echo 3. Visual Studio 17 2022 arm64
echo 4. Visual Studio 16 2019 Win32
echo 5. Visual Studio 16 2019 x64
echo 6. Visual Studio 15 2017
echo 7. Visual Studio 14 2015
echo 8. Visual Studio 12 2013
echo 9. Visual Studio 11 2012
echo a. Visual Studio 10 2010
echo b. Visual Studio 9 2008
rem echo c. Visual Studio 8 2005
rem echo d. Visual Studio NMake (experimental)
rem echo e. Cygwin MinGW Release + Unix Makefiles (experimental)
set /p no="select no "

echo %no%
if "%no%" == "f" set GENERATOR="Visual Studio 18 2026" & set OPT=-DARCHITECTURE=win32 & goto build_all
if "%no%" == "g" set GENERATOR="Visual Studio 18 2026" & set OPT=-DARCHITECTURE=x64   & goto build_all
if "%no%" == "h" set GENERATOR="Visual Studio 18 2026" & set OPT=-DARCHITECTURE=arm64 & goto build_all
if "%no%" == "1" set GENERATOR="Visual Studio 17 2022" & set OPT=-DARCHITECTURE=win32 & goto build_all
if "%no%" == "2" set GENERATOR="Visual Studio 17 2022" & set OPT=-DARCHITECTURE=x64   & goto build_all
if "%no%" == "3" set GENERATOR="Visual Studio 17 2022" & set OPT=-DARCHITECTURE=arm64 & goto build_all
if "%no%" == "4" set GENERATOR="Visual Studio 16 2019" & set OPT=-DARCHITECTURE=win32 & goto build_all
if "%no%" == "5" set GENERATOR="Visual Studio 16 2019" & set OPT=-DARCHITECTURE=x64   & goto build_all
if "%no%" == "6" set GENERATOR="Visual Studio 15 2017" & goto build_all
if "%no%" == "8" set GENERATOR="Visual Studio 14 2015" & goto build_all
if "%no%" == "8" set GENERATOR="Visual Studio 12 2013" & goto build_all
if "%no%" == "9" set GENERATOR="Visual Studio 11 2012" & goto build_all
if "%no%" == "a" set GENERATOR="Visual Studio 10 2010" & goto build_all
if "%no%" == "b" set GENERATOR="Visual Studio 9 2008" & goto build_all
if "%no%" == "c" set GENERATOR="Visual Studio 8 2005" & call :cmake_3_11_4 & goto build_all_2
if "%no%" == "d" set GENERATOR="NMake Makefiles" & set OPT=-DCMAKE_BUILD_TYPE=Release & goto build_all
if "%no%" == "e" set GENERATOR="Unix Makefiles" & set OPT=-DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake & goto build_all
echo ? retry
goto retry_vs

:build_all
if exist "%CMAKE_COMMAND%" goto build_all_2
where "%CMAKE_COMMAND%" 2> nul
if %errorlevel% == 0 goto build_all_2
echo cmake not found
pause
exit /b

:build_all_2
set C="%CMAKE_COMMAND%" -DCMAKE_GENERATOR=%GENERATOR% %OPT% -P buildall.cmake
echo %C%
title %C%
pause
%C%

:finish
echo build complete
endlocal
pause
exit /b

:cmake_3_11_4
set CMAKE_COMMAND=%~dp0..\buildtools\cmake-3.11.4-win32-x86\bin\cmake.exe
echo 1. PATH上のcmake.exeを使用する
echo 2. VS2005でも使用できるcmake使用する
echo    (必要なら自動でダウンロードして、このbuildtools/にインストールする)
if exist %CMAKE_COMMAND% echo    インストール済み(%CMAKE_COMMAND%)

set /p no="select no "
echo %no%
if "%no%" == "2" goto download

set CMAKE_COMMAND="cmake.exe"
where %CMAKE_COMMAND%
goto finish_cmake

:download
if exist %CMAKE_COMMAND% goto finish_cmake
call ..\buildtools\getcmake.bat

:finish_cmake
exit /b
