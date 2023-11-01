echo %~dp0\build_appveyor_release_bat_pre_cache.bat

set CUR=%~dp0
cd /d %CUR%..
del %CUR%..\libs\*.txt
del %CUR%..\libs\*.cmake
del %CUR%..\libs\*.bat
