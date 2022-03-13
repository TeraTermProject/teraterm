@echo off
setlocal
set CUR=%~dp0
cd /d %CUR%

set VS_VERSION=2019
set ONIG_VERSION=6.9.7.1
rem for 6.9.7.1
set ONIG_FOLDER_NAME=6.9.7
set ZLIB_VERSION=1.2.11
set PUTTY_VERSION=0.76
set SFMT_VERSION=1.5.1
set CJSON_VERSION=1.7.14
set ARGON2_VERSION=20190702
set LIBRESSL_VERSION=3.4.2

call :setup_tools_env

echo =======
echo 1. download libs, rebuild libs and Tera Term, installer, archive
echo 2. build libs
echo 3. build libs and rebuild Tera Term, installer, archive (for Release build)
echo 4. build libs and Tera Term (for Normal build, snapshot)
echo 7. exec cmd.exe
echo 8. check tools
echo 9. exit

if "%1" == "" (
    set /p no="select no "
) else (
    set no=%1
)
echo %no%

if "%no%" == "1" (
    call :update_libs
    call :build_teraterm freeze_state
)

if "%no%" == "2" (
    call :build_libs
)

if "%no%" == "3" (
    call :build_teraterm freeze_state
)

if "%no%" == "4" (
    call :build_teraterm
)

if "%no%" == "7" (
    call :exec_cmd
)

if "%no%" == "8" (
    call :check_tools
)

pause
exit 0


rem ####################
:update_libs

setlocal
cd /d %CUR%..\libs

:oniguruma
%CURL% -L https://github.com/kkos/oniguruma/releases/download/v%ONIG_VERSION%/onig-%ONIG_VERSION%.tar.gz -o oniguruma.tar.gz
%CMAKE% -E tar xf oniguruma.tar.gz
%CMAKE% -E rm -rf oniguruma
%CMAKE% -E rename onig-%ONIG_FOLDER_NAME% oniguruma

:zlib
%CURL% -L https://zlib.net/zlib-%ZLIB_VERSION%.tar.xz -o zlib.tar.xz
%CMAKE% -E tar xf zlib.tar.xz
%CMAKE% -E rm -rf zlib
%CMAKE% -E rename zlib-%ZLIB_VERSION% zlib

:putty
%CURL% -L https://the.earth.li/~sgtatham/putty/%PUTTY_VERSION%/putty-%PUTTY_VERSION%.tar.gz -o putty.tar.gz
%CMAKE% -E tar xf putty.tar.gz
%CMAKE% -E rm -rf putty
%CMAKE% -E rename putty-%PUTTY_VERSION% putty

:SFMT
%CURL% -L http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/SFMT-src-%SFMT_VERSION%.zip -o sfmt.zip
%CMAKE% -E tar xf sfmt.zip
%CMAKE% -E rm -rf SFMT
%CMAKE% -E rename SFMT-src-%SFMT_VERSION% SFMT
echo #define SFMT_VERSION "%SFMT_VERSION%" > SFMT\SFMT_version_for_teraterm.h

:cJSON
%CURL% -L https://github.com/DaveGamble/cJSON/archive/v%CJSON_VERSION%.zip -o cJSON.zip
%CMAKE% -E tar xf cJSON.zip
%CMAKE% -E rm -rf cJSON
%CMAKE% -E rename cJSON-%CJSON_VERSION% cJSON

:argon2
%CURL% -L https://github.com/P-H-C/phc-winner-argon2/archive/refs/tags/%ARGON2_VERSION%.tar.gz -o argon2.tar.gz
%CMAKE% -E tar xf argon2.tar.gz
%CMAKE% -E rm -rf argon2
%CMAKE% -E rename phc-winner-argon2-%ARGON2_VERSION% argon2

:libressl
%CURL% -L https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-%LIBRESSL_VERSION%.tar.gz -o libressl.tar.gz
%CMAKE% -E tar xf libressl.tar.gz
%CMAKE% -E rm -rf libressl
%CMAKE% -E rename libressl-%LIBRESSL_VERSION% libressl

endlocal
exit /b 0

rem ####################
:build_libs

setlocal
cd /d %CUR%..\libs
call buildall.bat
endlocal
exit /b 0

rem ####################
:build_teraterm

setlocal
cd /d %CUR%

if "%1" == "freeze_state" (
    call build.bat rebuild
    call makearchive.bat release
) else (
    call makearchive.bat
)
call ..\buildtools\svnrev\sourcetree_info.bat
if "%1" == "freeze_state" (
    pushd Output
    %CMAKE% -E tar cf TERATERM_r%SVNVERSION%_%DATE%_%TIME%.zip --format=zip teraterm-5.0/
    popd
) else (
    %CMAKE% -E tar cf TERATERM_r%SVNVERSION%_%DATE%_%TIME%.zip --format=zip snapshot-%DATE%_%TIME%
)
%INNO_SETUP% teraterm.iss

endlocal
exit /b 0

rem ####################
:setup_tools_env

set CURL=%SystemRoot%\System32\curl.exe
set CYGWIN_PATH=C:\cygwin64\bin
set VS_BASE=C:\Program Files (x86)\Microsoft Visual Studio\%VS_VERSION%

if exist toolinfo.bat (
    echo found toolinfo.bat
    call toolinfo.bat
    echo toolinfo.bat ok
) else (
    set PATH=
)

call :search_perl
call :search_svn
call :search_iscc
set PATH=%PATH%;%SVN_PATH%
set PATH=%PATH%;%PERL_PATH%
set PATH=%PATH%;%SystemRoot%
set PATH=%PATH%;%SystemRoot%\system32
call :set_vs_env
call :search_cmake
set PATH=%PATH%;%CYGWIN_PATH%
set PATH=%PATH%;%CMAKE_PATH%
exit /b 0

rem ####################
:search_perl
if exist %PERL_PATH%\perl.exe (
    set PERL=%PERL_PATH%\perl.exe
    exit /b 0
)

set PERL=perl.exe
where %PERL% > nul 2>&1
if %errorlevel% == 0 exit /b 0
set PERL=%CUR%..\buildtools\perl\perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Strawberry\perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Perl64\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\cygwin64\usr\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\cygwin\usr\bin\perl.exe
if exist %PERL% exit /b 0
echo perl not found
pause
exit

rem ####################
:search_svn
if exist %SVN_PATH%\svn.exe (
    set SVN=%SVN_PATH%\svn.exe
    exit /b 0
)

set SVN=svn.exe
where %SVN% > nul 2>&1
if %errorlevel% == 0 exit /b 0
set SVN_PATH=C:\Program Files (x86)\Subversion\bin
set SVN="%SVN_PATH%\svn.exe"
if exist %SVN% exit /b 0
set SVN_PATH=C:\Program Files\TortoiseSVN\bin
set SVN="%SVN_PATH%\svn.exe"
if exist %SVN% exit /b 0
echo svn not found
pause
exit

rem ####################
:search_cmake
if exist %CMAKE_PATH%\cmake.exe (
    set CMAKE="%CMAKE_PATH%\cmake.exe"
    exit /b 0
)

where %CMAKE% > nul 2>&1
if %errorlevel% == 0 exit /b 0
set CMAKE_PATH=C:\Program Files\CMake\bin
set CMAKE="%CMAKE_PATH%\cmake.exe"
if exist %CMAKE% exit /b 0
set CMAKE_PATH=%VCINSTALLDIR%\..\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
set CMAKE="%CMAKE_PATH%\cmake.exe"
if exist %CMAKE% exit /b 0
echo cmake not found
pause
exit

rem ####################
:search_iscc
if [%INNO_SETUP%] == [] goto search_iscc_1
if exist %INNO_SETUP% (
    exit /b 0
)
echo INNO_SETUP=%INNO_SETUP%
goto search_iscc_not_found

:search_iscc_1
set INNO_SETUP=%CUR%..\buildtools\innosetup6\bin\ISCC.exe
if exist %INNO_SETUP% exit /b 0
set INNO_SETUP="C:\Program Files (x86)\Inno Setup 6\iscc.exe"
if exist %INNO_SETUP% exit /b 0
:search_iscc_not_found
echo iscc(inno setup) not found
pause
exit

rem ####################
:set_vs_env

if exist "%VS_BASE%\Community" (
    call "%VS_BASE%\Community\VC\Auxiliary\Build\vcvars32.bat"
)
if exist "%VS_BASE%\Professional" (
    call "%VS_BASE%\Profssional\VC\Auxiliary\Build\vcvars32.bat"
)
if exist "%VS_BASE%\Enterprise" (
    call "%VS_BASE%\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
)
exit /b 0

rem ####################
:exec_cmd
cmd
exit /b 0

rem ####################
:check_tools

echo cmd(windows)
ver

echo Visual Studio
echo VS_BASE=%VS_BASE%
cl

echo curl
where curl
echo CURL=%CURL%
%CURL% --version

echo svn
where svn
echo SVN_PATH=%SVN_PATH%
echo SVN=%SVN%
svn --version

echo perl
where perl
echo PERL_PATH=%PERL_PATH%
echo PERL=%PERL%
perl --version

echo cmake
where cmake
echo CMAKE_PATH=%CMAKE_PATH%
echo CMAKE=%CMAKE%
%CMAKE% --version

echo cygwin
echo CYGWIN_PATH=%CYGWIN_PATH%
cygcheck -c base-cygwin
cygcheck -c gcc-core
cygcheck -c w32api-headers
cygcheck -c make

echo inno setup
echo INNO_SETUP=%INNO_SETUP%
%INNO_SETUP% /?

exit /b 0
