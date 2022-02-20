if exist teraterm\ttpdlg\svnversion.h del teraterm\ttpdlg\svnversion.h
if exist buildtools\svnrev\sourcetree_info.bat del buildtools\svnrev\sourcetree_info.bat
call ci_scripts\install.bat
call buildtools\svnrev\svnrev.bat
call buildtools\svnrev\sourcetree_info.bat
if exist c:\cygwin64\setup-x86_64.exe (
  c:\cygwin64\setup-x86_64.exe --quiet-mode --packages cmake --packages cygwin32-gcc-g++ --packages cygwin32-gcc-core
)
if exist c:\msys64\usr\bin\pacman.exe (
  c:\msys64\usr\bin\pacman.exe  -S --noconfirm --needed cmake
)
if "%GENERATOR%" == "Visual Studio 8 2005" (
  cd buildtools
  call getcmake.bat nopause
  cd ..
)
if "%COMPILER%" == "mingw"  (
  set PATH=C:\msys64\mingw32\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw-w64-i686-cmake mingw-w64-i686-gcc make
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw-w64-i686-clang
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE_OPTION_BUILD=-- -s -j
  set CMAKE_OPTION_GENERATE=%CMAKE_OPTION_GENERATE% -DCMAKE_BUILD_TYPE=Release
)
if "%COMPILER%" == "mingw_x64"  (
  set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc make
  pacman -S --noconfirm --needed mingw-w64-i686-cmake mingw-w64-i686-gcc make
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw-w64-x86_64-clang
    pacman -S --noconfirm --needed mingw-w64-i686-clang
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE_OPTION_BUILD=-- -s -j
  set CMAKE_OPTION_GENERATE=%CMAKE_OPTION_GENERATE% -DCMAKE_BUILD_TYPE=Release
)
cd libs
if not exist openssl11_%COMPILER% (
  "%CMAKE_COMMAND%" -DCMAKE_GENERATOR="%GENERATOR%" %CMAKE_OPTION_LIBS% -P buildall.cmake
  if exist build rmdir /s /q build
  if exist download rmdir /s /q download
  if exist openssl_%COMPILER%\html rmdir /s /q openssl_%COMPILER%\html
  if exist openssl_%COMPILER%_debug\html rmdir /s /q openssl_%COMPILER%_debug\html
  if exist ..\buildtools\perl\c rmdir /s /q ..\buildtools\perl\c
  if exist ..\buildtools\download rmdir /s /q ..\buildtools\download
)
cd ..
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%
if exist build_config.cmake del build_config.cmake
if exist cmakecache.txt del cmakecache.txt
set ZIP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
"%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE%
"%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD%
"%CMAKE_COMMAND%" --build . --target zip
"%CMAKE_COMMAND%" --build . --target inno_setup
cd ..
