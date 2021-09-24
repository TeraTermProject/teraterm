cmake_minimum_required(VERSION 3.13)

set(PACKAGE_NAME "cygtool_dll")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME} SHARED
  cygtool.c
  cygtool.def
  )

if(MINGW)
  # libgcc_s_dw2-1.dll に依存しないため
  target_link_options(
    ${PACKAGE_NAME}
    PRIVATE
    -static-libgcc
    )
endif()

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/libmingw/tlssup.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  version
  )

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  OUTPUT_NAME "cygtool"
  )

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  FOLDER installer
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
  )

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )

if(false)
  add_executable(
    cygtool_exe
    cygtool.c
    )

  target_compile_options(
    cygtool_exe
    PRIVATE
    -D EXE
    -I ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
    )

  target_link_libraries(
    cygtool_exe
    PRIVATE
    version
    )

  set_target_properties(
    cygtool_exe
    PROPERTIES
    OUTPUT_NAME "cygtool"
    )

  set_target_properties(
    cygtool_exe
    PROPERTIES
    FOLDER installer
    )
endif()
