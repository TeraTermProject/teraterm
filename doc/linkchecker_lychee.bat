@echo off
setlocal
pushd %~dp0

if "%CMAKE_COMMAND%" == "" (
   call ..\ci_scripts\find_cmake.bat
)

"%CMAKE_COMMAND%" -P linkchecker_lychee.cmake
set RESULT=%errorlevel%

popd

if not "%NOPAUSE%" == "1" pause
exit /b %RESULT%
