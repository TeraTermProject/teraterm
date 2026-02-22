rem Build zlib
setlocal

pushd zlib


rem architecture for VsDevCmd.bat
if "%TARGET%" == "" (set TARGET=Win32)
if "%TARGET%" == "Win32" (set ARCHITECTURE=x86)
if "%TARGET%" == "x64"   (set ARCHITECTURE=x64)
if "%TARGET%" == "ARM64" (set ARCHITECTURE=arm64)
if "%TARGET%" == "ARM64" if "%HOST_ARCHITECTURE%" == "" (set HOST_ARCHITECTURE=amd64)


rem Find Visual Studio
if not "%VSINSTALLDIR%" == "" goto vsinstdir

:check_2019
if "%VS160COMNTOOLS%" == "" goto check_2022
if not exist "%VS160COMNTOOLS%\VsDevCmd.bat" goto check_2022
call "%VS160COMNTOOLS%\VsDevCmd.bat" -arch=%ARCHITECTURE% -host_arch=%HOST_ARCHITECTURE%
goto vs2019

:check_2022
if "%VS170COMNTOOLS%" == "" goto novs
if not exist "%VS170COMNTOOLS%\VsDevCmd.bat" goto novs
call "%VS170COMNTOOLS%\VsDevCmd.bat" -arch=%ARCHITECTURE% -host_arch=%HOST_ARCHITECTURE%
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
if /I %VSCMNDIR% EQU "%VS180COMNTOOLS%" goto vs2026

echo Unknown Visual Studio version
goto fail


rem Generate Makefile
rem -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded" を指定すると、Debug が MultiThreadedDebug にならず MultiThreaded になるので指定しない
:vs2019
cmake -G "Visual Studio 16 2019" -A %TARGET% -S . -B build\%TARGET%
goto gen_end

:vs2022
cmake -G "Visual Studio 17 2022" -A %TARGET% -S . -B build\%TARGET%
goto gen_end

:vs2026
cmake -G "Visual Studio 18 2026" -A %TARGET% -S . -B build\%TARGET%
goto gen_end

:gen_end

rem libz must be /MT(d)
perl -e "open(IN,'build\%TARGET%\zlibstatic.vcxproj');while(<IN>){s/MultiThreadedDebugDLL/MultiThreadedDebug/;print $_;}close(IN);" > build\%TARGET%\zlibstatic.vcxproj.tmp
perl -e "open(IN,'build\%TARGET%\zlibstatic.vcxproj.tmp');while(<IN>){s/MultiThreadedDLL/MultiThreaded/;print $_;}close(IN);" > build\%TARGET%\zlibstatic.vcxproj.tmp2
COPY build\%TARGET%\zlibstatic.vcxproj.tmp2 build\%TARGET%\zlibstatic.vcxproj
DEL build\%TARGET%\zlibstatic.vcxproj.tmp
DEL build\%TARGET%\zlibstatic.vcxproj.tmp2


rem Build
cmake --build build\%TARGET% --target zlibstatic --config Debug
cmake --build build\%TARGET% --target zlibstatic --config Release


:end
popd
endlocal
exit /b 0


:fail
popd
echo "buildzlib.bat failed"
@echo on
endlocal
exit /b 1
