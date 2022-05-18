set CUR=%~dp0
cd /d %CUR%..
del %CUR%..\libs\*.txt
del %CUR%..\libs\*.cmake
del %CUR%..\libs\*.bat
