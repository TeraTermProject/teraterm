@echo off
setlocal
set CUR=%~dp0
cd /d %CUR%

set NOPAUSE=1
call ..\buildtools\install_lychee.bat
set LYCHEE=%CUR%..\buildtools\lychee\lychee.exe

rem git 管理下の *.md のリンクをチェックする
rem (md は UTF-8 なので変換不要、そのまま lychee に渡す)
cd /d %CUR%..
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
 "& '%LYCHEE%' @(git ls-files '*.md'); exit $LASTEXITCODE"
set RESULT=%errorlevel%

pause
exit /b %RESULT%
