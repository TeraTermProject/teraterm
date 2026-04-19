cmake_minimum_required(VERSION 3.11)

project("TTXChangeFontSize")

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
)

add_library(
  ${PROJECT_NAME} SHARED
  TTXChangeFontSize.c
  ReadMe.txt
  ReadMe-ja.txt
  LICENSE
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  common_static
)

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_link_libraries(
      ${PROJECT_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      mingw_dll_tls
      -Wl,--no-whole-archive
    )
  endif()
endif(SUPPORT_OLD_WINDOWS)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
)

install(
  TARGETS ${PROJECT_NAME}
  RUNTIME
  DESTINATION .
)
