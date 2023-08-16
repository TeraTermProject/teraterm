echo %~dp0\install.bat

pushd %~dp0

if "%CMAKE_COMMAND%" == "" (
   call find_cmake.bat
)

"%CMAKE_COMMAND%" -P ../buildtools/install_cygwin.cmake

if exist c:\msys64\usr\bin\pacman.exe (
  c:\msys64\usr\bin\pacman.exe  -S --noconfirm --needed cmake
)

popd
