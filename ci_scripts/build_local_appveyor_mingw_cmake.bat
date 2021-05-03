setlocal
set PATH=C:\msys64\mingw32\bin;C:\msys64\usr\bin
cmake -P build_local_appveyor_mingw.cmake
rem cmake -DCOMPILER_GCC=ON -DCOMPILER_CLANG=OFF -DCOMPILER_64BIT=OFF -P build_local_appveyor_mingw.cmake
rem cmake -DCOMPILER_GCC=OFF -DCOMPILER_CLANG=ON -DCOMPILER_64BIT=OFF -P build_local_appveyor_mingw.cmake
rem cmake -DCOMPILER_GCC=ON -DCOMPILER_CLANG=OFF -DCOMPILER_64BIT=ON -P build_local_appveyor_mingw.cmake
rem cmake -DCOMPILER_GCC=OFF -DCOMPILER_CLANG=ON -DCOMPILER_64BIT=ON -P build_local_appveyor_mingw.cmake
pause
