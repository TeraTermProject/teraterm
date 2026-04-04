cmake_minimum_required(VERSION 3.11)

project("cygtool")

add_library(
  ${PROJECT_NAME} SHARED
  cygtool.c
  cygtool.h
  )
if(MSVC)
  target_sources(
    ${PROJECT_NAME}
    PRIVATE
    cygtool.def
    )
endif()

if(MINGW)
  # libgcc_s_dw2-1.dll に依存しないため
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
    -static-libgcc
    -Wl,--add-stdcall-alias
  )
endif()

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/libmingw/tlssup.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

if(MINGW)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES PREFIX ""
    SUFFIX ".dll"
    )
endif()

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  version
  )

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
  )

install(
  TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION .
  )

###### cygtool_tester

add_executable(
  cygtool_tester
  cygtool_tester.c
  )

target_link_libraries(
  cygtool_tester
  PRIVATE
  ${PROJECT_NAME}
  )
