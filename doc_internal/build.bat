cd /d %~dp0
call ..\ci_scripts\find_cmake.bat
"%CMAKE_COMMAND%" -P doxygen.cmake
"%CMAKE_COMMAND%" -P global.cmake
pause
