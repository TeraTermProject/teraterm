echo %~dp0\addpkg_msys2.bat

pushd %~dp0

if exist c:\msys64\usr\bin\pacman.exe (
  c:\msys64\usr\bin\pacman.exe  -S --noconfirm --needed cmake
)

popd
