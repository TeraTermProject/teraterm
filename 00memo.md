
# cygwin
cmake -DCMAKE_FIND_ROOT_PATH=/usr/i686-w64-mingw32 -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ -DCMAKE_RC_COMPILER=i686-w64-mingw32-windres .. -G "Unix Makefiles"

cmake -DCMAKE_FIND_ROOT_PATH=/usr/i686-w64-mingw32 -DCMAKE_C_COMPILER=i686-w64-mingw32-clang -DCMAKE_CXX_COMPILER=i686-w64-mingw32-clang++ -DCMAKE_RC_COMPILER=i686-w64-mingw32-windres .. -G "Unix Makefiles"
