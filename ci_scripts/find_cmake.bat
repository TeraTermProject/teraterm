"%SystemRoot%\system32\where.exe" %CMAKE_COMMAND% 2> nul > nul && exit /b

set CMAKE_COMMAND=C:\Program Files\CMake\bin\cmake.exe
if exist "%CMAKE_COMMAND%" exit /b

set CMAKE_DIR=C:\Program Files (x86)\Microsoft Visual Studio\2019
call :search_vs
if %errorlevel% == 0 exit /b
set CMAKE_DIR=C:\Program Files (x86)\Microsoft Visual Studio\2017
call :search_vs
if %errorlevel% == 0 exit /b

echo Install cmake
pause
exit

:search_vs
set CMAKE_COMMAND=%CMAKE_DIR%\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
if exist "%CMAKE_COMMAND%" exit /b 1
set CMAKE_COMMAND=%CMAKE_DIR%\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
if exist "%CMAKE_COMMAND%" exit /b 1
set CMAKE_COMMAND=%CMAKE_DIR%\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
if exist "%CMAKE_COMMAND%" exit /b 1
exit /b 0

