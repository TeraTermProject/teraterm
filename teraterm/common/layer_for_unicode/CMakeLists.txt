cmake_minimum_required(VERSION 3.11)

set(PACKAGE_NAME "layer_for_unicode")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME}
  STATIC
  init_ptr.cpp
  ../layer_for_unicode.cpp
  ../layer_for_unicode.h
  ../layer_for_unicode_comctl32.cpp
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ..
  )

if(NOT EXISTS "list_asm.cmake")
  execute_process(
    COMMAND ${PERL} generate.pl
    RESULT_VARIABLE rv
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
  if(NOT rv STREQUAL "0")
    message(FATAL_ERROR "execute error ${PERL}")
  endif()
endif()

if(${CMAKE_VERSION} VERSION_LESS "3.12.0")

  set(OBJ_DIR ".")
  include("list_obj.cmake")

  target_sources(
    ${PACKAGE_NAME}
    PRIVATE
    ${SRC}
    )

else()
  # VS2010+

  include("list_asm.cmake")

  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    set(BUILDTOOLS_NASM ${CMAKE_CURRENT_LIST_DIR}/../../../buildtools/nasm/nasm.exe)
    if(EXISTS ${BUILDTOOLS_NASM})
      set(CMAKE_ASM_NASM_COMPILER ${BUILDTOOLS_NASM})
    endif()
  endif()
  set(CMAKE_ASM_NASM_OBJECT_FORMAT win32)
  set(CMAKE_ASM_NASM_FLAGS_DEBUG "-g -Fcv8")

  enable_language(ASM_NASM)

  target_sources(
    ${PACKAGE_NAME}
    PRIVATE
    ${SRC}
    )

  source_group(
    "assember"
    REGULAR_EXPRESSION
    "asm")
endif()

if(${CMAKE_VERSION} VERSION_GREATER "3.12.0")
  target_link_libraries(
    ${PACKAGE_NAME}
    PUBLIC
    common_static
    $<$<CONFIG:Release>:libucrt.lib libvcruntime.lib>
    $<$<CONFIG:Debug>:libucrtd.lib libvcruntimed.lib>
    )
else()
  target_link_libraries(
    ${PACKAGE_NAME}
    PUBLIC
    $<$<CONFIG:Release>:LIBCMT.lib LIBCPMT.lib>
    $<$<CONFIG:Debug>:LIBCMTd.lib LIBCPMTd.lib>
    )
endif()

target_link_libraries(
  ${PACKAGE_NAME}
  PUBLIC
  oleaut32
  advapi32
  ole32
  user32
  kernel32
  gdi32
  comdlg32
  comctl32
  shell32
  uuid
  )
