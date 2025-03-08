echo %~dp0\build_appveyor_release_bat.bat

set CUR=%~dp0

cd /d %CUR%..
call installer\release.bat 2
if ERRORLEVEL 1 (
	echo installer\release.bat 2
	exit 1
)
