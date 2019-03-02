@echo off
setlocal
cd /d %~dp0
echo cmakeをダウンロードしてlib/cmakeに展開します
pause
powershell -NoProfile -ExecutionPolicy Unrestricted .\getcmake.ps1
endlocal
pause
