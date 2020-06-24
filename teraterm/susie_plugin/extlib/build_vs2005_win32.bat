set CMAKE=..\..\..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe

%CMAKE% -DGENERATE_OPTION="-G;Visual Studio 8 2005" -DINSTALL_PREFIX_ADD="vs2005_win32/" -P extlib.cmake
pause
