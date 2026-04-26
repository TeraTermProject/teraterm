cmake_minimum_required(VERSION 3.10)
project(libcompatwin)

set(CMAKE_ASM_NASM_COMPILER ${CMAKE_CURRENT_LIST_DIR}/../buildtools/nasm/nasm.exe)
set(CMAKE_ASM_NASM_OBJECT_FORMAT win32)
set(CMAKE_ASM_NASM_FLAGS_DEBUG "-g -Fcv8")
enable_language(ASM_NASM)

add_library(${PROJECT_NAME}
  STATIC
  initonceexeconce.cpp
  InitOnceExecuteOnce.asm
  inet_pton.cpp
  inet_pton.asm
  libcompatwin_init.cpp
  BCryptGenRandom.asm
  BCryptGenRandom.cpp
)

source_group(
  "asm"
  REGULAR_EXPRESSION
  "asm")

if(MSVC)
  target_compile_options(${PROJECT_NAME}
    PRIVATE
    "$<$<CONFIG:Debug>:/ZI>"
  )
endif()
