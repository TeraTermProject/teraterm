set CMAKE=..\..\..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe

mkdir build_vs2005_win32
cd build_vs2005_win32
%CMAKE% -DEXTLIB=extlib/vs2005_win32 -G "Visual Studio 8 2005" ..
%CMAKE% --build . --config Release
pause
