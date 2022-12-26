@echo off
setlocal
cd /d %~dp0
echo perlをダウンロードしてbuildtools/perlに展開します
pause
IF NOT EXIST "C:\Program Files\CMake\bin" goto by_powershell

:by_cmake
set PATH=C:\Program Files\CMake\bin;%PATH%
cmake -P getperl.cmake
goto finish

:by_powershell
powershell -NoProfile -ExecutionPolicy Unrestricted .\getperl.ps1
goto finish

:finish
endlocal
pause
