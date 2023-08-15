echo %~dp0\install.bat

cd /d %~dp0

setlocal
if "%CMAKE_COMMAND%" == "" (
   call find_cmake.bat
)

"%CMAKE_COMMAND%" -P ../buildtools/install_cygwin.cmake

if exist c:\msys64\usr\bin\pacman.exe (
  c:\msys64\usr\bin\pacman.exe  -S --noconfirm --needed cmake
)

rem ‚±‚±‚Å cd ‚µ‚Ä‚àˆÚ“®‚³‚ê‚È‚¢
rem install.bat ‚ğŒÄ‚Ño‚µ‚½‘¤‚Å–ß‚µ‚Ä‚à‚ç‚¤
rem cd ..
