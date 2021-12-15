echo %~dp0\install.bat
setlocal
if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2013" (
   cd %~dp0
   "%CMAKE_COMMAND%" -P install_cygwin.cmake
)
