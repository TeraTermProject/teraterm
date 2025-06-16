echo %~dp0\build_appveyor_release_bat_pre_cache.bat

rem キャッシュに不要なファイルを削除する
set CUR=%~dp0
cd /d %CUR%..
del %CUR%..\libs\*.txt
del %CUR%..\libs\*.cmake
del %CUR%..\libs\*.bat
del %CUR%..\buildtools\*.md
del %CUR%..\buildtools\*.cmake
rmdir /s /q %CUR%..\buildtools\svnrev
rmdir /s /q %CUR%..\buildtools\docker
rmdir /s /q %CUR%..\buildtools\download\cygwin_package
rmdir /s /q %CUR%..\buildtools\download\cygwin32_package
if exist %CUR%..\buildtools\download\innosetup6 (
    rmdir /s /q %CUR%..\buildtools\download\innosetup6
)
