@echo off
setlocal
cd /d %~dp0
echo perlをダウンロードしてlib/perlに展開します
pause
powershell -NoProfile -ExecutionPolicy Unrestricted .\getperl.ps1
endlocal
pause
