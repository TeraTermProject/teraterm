# how to build:
# mkdir build; cd build
# cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
# cmake --build .

# target
set(CMAKE_SYSTEM_NAME Windows)

# option
option(USE_CLANG "use clang compiler" OFF)

# mingw on msys
#set(CMAKE_SYSROOT /mingw32/i686-w64-mingw32)
#set(CMAKE_FIND_ROOT_PATH /mingw32/i686-w64-mingw32)

# mingw
set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
#set(CMAKE_SYSROOT /usr/i686-w64-mingw32)

if(USE_CLANG)
  set(CMAKE_C_COMPILER i686-w64-mingw32-clang)
  set(CMAKE_CXX_COMPILER i686-w64-mingw32-clang++)
else()
  set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
  set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
endif()
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} CACHE PATH "toolchain file")

