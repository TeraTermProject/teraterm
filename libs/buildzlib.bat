rem Build zlib

pushd zlib


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
rem -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded" を指定すると、Debug が MultiThreadedDebug にならず MultiThreaded になるので指定しない
:vs2019
cmake -G "Visual Studio 16 2019" -A Win32 -S . -B build\Win32
goto gen_end

:vs2022
cmake -G "Visual Studio 17 2022" -A Win32 -S . -B build\Win32
goto gen_end

:gen_end

rem libz must be /MT(d)
perl -e "open(IN,'build\Win32\zlibstatic.vcxproj');while(<IN>){s/MultiThreadedDebugDLL/MultiThreadedDebug/;print $_;}close(IN);" > build\Win32\zlibstatic.vcxproj.tmp
perl -e "open(IN,'build\Win32\zlibstatic.vcxproj.tmp');while(<IN>){s/MultiThreadedDLL/MultiThreaded/;print $_;}close(IN);" > build\Win32\zlibstatic.vcxproj.tmp2
COPY build\Win32\zlibstatic.vcxproj.tmp2 build\Win32\zlibstatic.vcxproj
DEL build\Win32\zlibstatic.vcxproj.tmp
DEL build\Win32\zlibstatic.vcxproj.tmp2


rem Build
cmake --build build\Win32 --target zlibstatic --config Debug
cmake --build build\Win32 --target zlibstatic --config Release


:end
popd
exit /b 0


:fail
popd
echo "buildzlib.bat failed"
@echo on
exit /b 1
