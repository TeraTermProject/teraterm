rem LibreSSLのビルド

cd libressl


if exist "CMakeCache.txt" goto end


if not "%VSINSTALLDIR%" == "" goto vsinstdir

:check_2019
if "%VS160COMNTOOLS%" == "" goto check_2022
if not exist "%VS160COMNTOOLS%\VsDevCmd.bat" goto check_2022
call "%VS160COMNTOOLS%\VsDevCmd.bat"
goto vs2019

:check_2022
if "%VS170COMNTOOLS%" == "" goto novs
if not exist "%VS170COMNTOOLS%\VsDevCmd.bat" goto novs
call "%VS170COMNTOOLS%\VsDevCmd.bat"
goto vs2022

:novs
echo "Can't find Visual Studio"
goto fail

:vsinstdir
rem Visual Studioのバージョン判別
set VSCMNDIR="%VSINSTALLDIR%\Common7\Tools\"
set VSCMNDIR=%VSCMNDIR:\\=\%

if /I %VSCMNDIR% EQU "%VS160COMNTOOLS%" goto vs2019
if /I %VSCMNDIR% EQU "%VS170COMNTOOLS%" goto vs2022

echo Unknown Visual Studio version
goto fail

:vs2019
set CMAKE_PARAMETER=-G "Visual Studio 16 2019" -A Win32
goto vsend

:vs2022
set CMAKE_PARAMETER=-G "Visual Studio 17 2022" -A Win32
goto vsend

:vsend

set CMAKE=cmake
rem set CMAKE="C:\Program Files\CMake\bin\cmake"
rem set CMAKE="%VSINSTALLDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake"

%CMAKE% -DMSVC=on -DUSE_STATIC_MSVC_RUNTIMES=on %CMAKE_PARAMETER%

devenv /build Debug LibreSSL.sln /project crypto /projectconfig Debug

devenv /build Release LibreSSL.sln /project crypto /projectconfig Release


:end
cd ..
exit /b 0


:fail
cd ..
echo "buildlibressl.bat を終了します"
@echo on
exit /b 1
