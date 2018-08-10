# how to build:
# mkdir build; cd build
# cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
# cmake --build .

set(CMAKE_SYSTEM_NAME Windows)

if(0)
  set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
  if(1)
	set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
	set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
  endif()
  if(0)
	set(CMAKE_C_COMPILER x86_64-w64-mingw32-clang)
	set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-clang++)
  endif()
  set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
else()
  set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
  set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
  set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
  set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -ffunction-sections" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -ffunction-sections" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s" CACHE STRING "")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s" CACHE STRING "")

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} CACHE PATH "toolchain file")
