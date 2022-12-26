cd /d %~dp0
set PATH=C:\Program Files\CMake\bin;%PATH%
cmake -P install_cygwin.cmake
pause

