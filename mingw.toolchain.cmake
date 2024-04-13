# how to build:
# mkdir build; cd build
# cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
# cmake --build .

# target
set(CMAKE_SYSTEM_NAME Windows)

# option
option(USE_GCC_32   "use gcc 32bit compiler" OFF)
option(USE_GCC_64   "use gcc 64bit compiler" OFF)
option(USE_CLANG_32 "use clang 32bit compiler" OFF)
option(USE_CLANG_64 "use clang 64bit compiler" OFF)

if((NOT USE_GCC_32) AND (NOT USE_GCC_64) AND
    (NOT USE_CLANG_32) AND (NOT USE_CLANG_64))
  # default compiler
  set(USE_GCC_32 ON)
endif()

if(USE_GCC_32 OR USE_CLANG_32)
  set(CMAKE_SYSTEM_PROCESSOR i686)
  set(PREFIX "i686-w64-mingw32-")
  set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
endif()
if(USE_GCC_64 OR USE_CLANG_64)
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
  set(PREFIX "x86_64-w64-mingw32-")
  set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
endif()

set(THREAD_MODEL "-win32")
#set(THREAD_MODEL "-posix")
if(${CMAKE_COMMAND} MATCHES "msys2")
  # msys2にはposix版のみ
  unset(THREAD_MODEL)
endif()

if(USE_GCC_32 OR USE_GCC_64)
  set(CMAKE_C_COMPILER   ${PREFIX}gcc${THREAD_MODEL})
  set(CMAKE_CXX_COMPILER ${PREFIX}g++${THREAD_MODEL})
  set(CMAKE_RC_COMPILER  ${PREFIX}windres)
elseif(USE_CLANG_32 OR USE_CLANG_64)
  set(CMAKE_C_COMPILER   ${PREFIX}clang${THREAD_MODEL})
  set(CMAKE_CXX_COMPILER ${PREFIX}clang++${THREAD_MODEL})
  set(CMAKE_RC_COMPILER  ${PREFIX}windres)
else()
  message(FATAL_ERROR "check compiler")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} CACHE PATH "toolchain file")
