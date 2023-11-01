echo %~dp0\build_appveyor_release_bat.bat

set CUR=%~dp0

cd /d %CUR%..
call installer\release.bat 2
