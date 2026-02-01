# how to build:
# mkdir build; cd build
# cmake .. -G "Unix Makefiles" -DARCHITECTURE=i686 -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
# cmake .. -G "Unix Makefiles" -DARCHITECTURE=x86_64 -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
# cmake .. -G Ninja -DARCHITECTURE=x86_64 -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
cmake_policy(SET CMP0053 NEW)

# target
set(CMAKE_SYSTEM_NAME Windows)

if(NOT DEFINED ARCHITECTURE)
  set(ARCHITECTURE "i686")
#  set(ARCHITECTURE "x86_64")
endif()

# option
option(USE_CLANG "use clang compiler" OFF)

if(${ARCHITECTURE} STREQUAL "i686")
  set(CMAKE_SYSTEM_PROCESSOR i686)
  set(PREFIX "i686-w64-mingw32-")
  set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
elseif(${ARCHITECTURE} STREQUAL "x86_64")
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
  set(ARCHITECTURE "x86_64")
  set(PREFIX "x86_64-w64-mingw32-")
  set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
else()
  message(FATAL_ERROR "ARCHITECTURE=${ARCHITECTURE}")
endif()

set(THREAD_MODEL "-win32")
#set(THREAD_MODEL "-posix")
if(MSYS OR (${CMAKE_COMMAND} MATCHES "msys2") OR (${CMAKE_COMMAND} MATCHES "mingw") OR (${CMAKE_COMMAND} MATCHES "clang64"))
  # msys2はposix版のみ
  unset(THREAD_MODEL)
elseif(CYGWIN OR (${CMAKE_HOST_SYSTEM_NAME} MATCHES "CYGWIN"))
  # Cygwinはposix版のみ
  unset(THREAD_MODEL)
endif()

if(NOT USE_CLANG)
  set(CMAKE_C_COMPILER   ${PREFIX}gcc${THREAD_MODEL})
  set(CMAKE_CXX_COMPILER ${PREFIX}g++${THREAD_MODEL})
  set(CMAKE_RC_COMPILER  ${PREFIX}windres)
else()
  set(CMAKE_C_COMPILER   ${PREFIX}clang${THREAD_MODEL})
  set(CMAKE_CXX_COMPILER ${PREFIX}clang++${THREAD_MODEL})
  set(CMAKE_RC_COMPILER  ${PREFIX}windres)
endif()
if(MSYS OR (${CMAKE_COMMAND} MATCHES "msys2") OR (${CMAKE_COMMAND} MATCHES "mingw") OR (${CMAKE_COMMAND} MATCHES "clang64"))
  set(CMAKE_RC_COMPILER windres)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} CACHE PATH "toolchain file")
