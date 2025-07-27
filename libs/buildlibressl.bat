rem Build LibreSSL

pushd libressl


rem Find Visual Studio
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
rem Check Visual Studio version
set VSCMNDIR="%VSINSTALLDIR%\Common7\Tools\"
set VSCMNDIR=%VSCMNDIR:\\=\%

if /I %VSCMNDIR% EQU "%VS160COMNTOOLS%" goto vs2019
if /I %VSCMNDIR% EQU "%VS170COMNTOOLS%" goto vs2022

echo Unknown Visual Studio version
goto fail


rem Generate Makefile
:vs2019
set CMAKE_PARAMETER=-G "Visual Studio 16 2019" -A Win32 -B build\Win32
goto gen_end

:vs2022
set CMAKE_PARAMETER=-G "Visual Studio 17 2022" -A Win32 -B build\Win32
goto gen_end

:gen_end

cmake -DMSVC=on -DUSE_STATIC_MSVC_RUNTIMES=on %CMAKE_PARAMETER%
cmake --build build\Win32 --target crypto --config Debug
cmake --build build\Win32 --target crypto --config Release


:end
popd
exit /b 0


:fail
popd
echo "buildlibressl.bat failed"
@echo on
exit /b 1
