# macOS ARM64 (Apple Silicon) toolchain for Tera Term
#
# Usage:
#   mkdir build_macos && cd build_macos
#   cmake .. -G "Unix Makefiles" -DARCHITECTURE=arm64 -DCMAKE_TOOLCHAIN_FILE=../macos.toolchain.cmake
#   cmake .. -G Ninja -DARCHITECTURE=arm64 -DCMAKE_TOOLCHAIN_FILE=../macos.toolchain.cmake
#
# For x86_64 (Intel Mac):
#   cmake .. -G "Unix Makefiles" -DARCHITECTURE=x86_64 -DCMAKE_TOOLCHAIN_FILE=../macos.toolchain.cmake

cmake_policy(SET CMP0053 NEW)

set(CMAKE_SYSTEM_NAME Darwin)

if(NOT DEFINED ARCHITECTURE)
  # Default to ARM64 on Apple Silicon
  execute_process(
    COMMAND uname -m
    OUTPUT_VARIABLE HOST_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(HOST_ARCH STREQUAL "arm64")
    set(ARCHITECTURE "arm64")
  else()
    set(ARCHITECTURE "x86_64")
  endif()
endif()

if(${ARCHITECTURE} STREQUAL "arm64")
  set(CMAKE_SYSTEM_PROCESSOR arm64)
  set(CMAKE_OSX_ARCHITECTURES arm64)
elseif(${ARCHITECTURE} STREQUAL "x86_64")
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
  set(CMAKE_OSX_ARCHITECTURES x86_64)
elseif(${ARCHITECTURE} STREQUAL "universal")
  set(CMAKE_SYSTEM_PROCESSOR arm64)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
else()
  message(FATAL_ERROR "Unsupported ARCHITECTURE=${ARCHITECTURE}. Use arm64, x86_64, or universal.")
endif()

# Minimum macOS deployment target (macOS 11.0 for ARM64 support)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")

# Use Apple Clang
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} CACHE PATH "toolchain file")
