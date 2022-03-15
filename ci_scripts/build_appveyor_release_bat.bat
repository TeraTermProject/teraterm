set CUR=%~dp0
cd /d %CUR%..
call ci_scripts\install.bat
call installer\release.bat 2
