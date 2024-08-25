echo %~dp0\install_cygwin.bat

pushd %~dp0

if "%CMAKE_COMMAND%" == "" (
   call ..\ci_scripts\find_cmake.bat
)
"%CMAKE_COMMAND%" -P install_cygwin.cmake
rem "%CMAKE_COMMAND%" -DREMOVE_TMP=ON -P install_cygwin.cmake

popd

if not "%NOPAUSE%" == "1" pause
