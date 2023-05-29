setlocal
cd /d %~dp0
if "%CMAKE_COMMAND%" == "" (
   call ..\ci_scripts\find_cmake.bat
)
"%CMAKE_COMMAND%" -P ../buildtools/install_cygwin.cmake
rem "%CMAKE_COMMAND%" -DREMOVE_TMP=ON -P ../buildtools/install_cygwin.cmake
pause

