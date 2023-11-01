echo %~dp0\install_mycygwin.bat

pushd %~dp0

if "%CMAKE_COMMAND%" == "" (
   call find_cmake.bat
)

"%CMAKE_COMMAND%" -P ../buildtools/install_cygwin.cmake

popd
