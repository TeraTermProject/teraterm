cd %~dp0
setlocal
set NOPAUSE=1
call install_cygwin.bat
call install_msys2.bat
call install_sbapplocale.bat
call install_innosetup.bat
call install_mingw32.bat
call mingw32.bat
"%CMAKE_COMMAND%" -P nasm.cmake
cd ..
cd libs
if not exist libressl_mingw_i686 goto build_libs
set BUILD_LIBS=n
set /p BUILD_LIBS="libs/libressl_mingw_i686 exists. build libs? (y/n) "
if /i not "%BUILD_LIBS%" == "y" goto skip_libs
:build_libs
echo "%CMAKE_COMMAND%" -DCMAKE_GENERATOR="MinGW Makefiles" -DARCHITECTURE=i686 -P buildall.cmake
pause
"%CMAKE_COMMAND%" -DCMAKE_GENERATOR="MinGW Makefiles" -DARCHITECTURE=i686 -P buildall.cmake
:skip_libs
cd ..
set BUILD_DIR=build_mingw32
set PATH=%CD%\buildtools\mingw32\bin;C:\Program Files\Git\cmd;C:\Windows\System32;C:\Windows
"%CMAKE_COMMAND%" -S . -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=%CD%\buildtools\mingw32\bin\mingw32-make.exe -DARCHITECTURE=i686 -DSUPPORT_OLD_WINDOWS=ON -DCMAKE_BUILD_TYPE=Release
"%CMAKE_COMMAND%" --build %BUILD_DIR% -j --target install
%BUILD_DIR%\make_installer_cmake.bat
pause
