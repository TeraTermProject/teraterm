setlocal
cd /d %~dp0
if "%CMAKE_COMMAND%" == "" (
   call ..\ci_scripts\find_cmake.bat
)
"%CMAKE_COMMAND%" -P install_innosetup.cmake
pause
