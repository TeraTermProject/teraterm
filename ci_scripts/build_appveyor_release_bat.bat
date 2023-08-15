set CUR=%~dp0

cd /d %CUR%..
call ci_scripts\install.bat

cd /d %CUR%..
call installer\release.bat 2
