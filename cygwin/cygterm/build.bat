setlocal
rem PATH=c:\cygwin\bin
PATH=c:\cygwin64\bin
uname -a > build_info.txt
make EXE=cygterm_i686.exe clean
make -j cygterm_i686
make EXE=cygterm_x86_64.exe clean
make -j cygterm_x86_64
file *.exe >> build_info.txt
pause
