cd /d %~dp0..
if exist teraterm\ttpdlg\svnversion.h del teraterm\ttpdlg\svnversion.h
if exist buildtools\svnrev\sourcetree_info.bat del buildtools\svnrev\sourcetree_info.bat
rem call ci_scripts\install_mycygwin.bat
cd /d %~dp0..
call buildtools\svnrev\svnrev.bat
call buildtools\svnrev\sourcetree_info.bat
echo GENERATOR=%GENERATOR%
echo CMAKE_OPTION_LIBS=%CMAKE_OPTION_LIBS%
echo CMAKE_COMMAND=%CMAKE_COMMAND%
echo BUILD_DIR=%BUILD_DIR%
pause
if "%COMPILER%" == "mingw"  (
  set PATH=C:\msys64\mingw32\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw-w64-i686-cmake mingw-w64-i686-gcc make
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw-w64-i686-clang
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE=C:\msys64\mingw64\bin\cmake.exe
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
if exist libs\omit_build_libs_%COMPILER% goto omit_build_libs
cd libs
"%CMAKE_COMMAND%" -DCMAKE_GENERATOR="%GENERATOR%" %CMAKE_OPTION_LIBS% -P buildall.cmake
rem if exist build rmdir /s /q build
rem if exist download rmdir /s /q download
cd ..
:omit_build_libs
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%
if exist build_config.cmake del build_config.cmake
if exist cmakecache.txt del cmakecache.txt
set ZIP_FILE=teraterm-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=teraterm-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=teraterm-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
"%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE%
"%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD%
"%CMAKE_COMMAND%" --build . --target zip
"%CMAKE_COMMAND%" --build . --target inno_setup
cd ..
