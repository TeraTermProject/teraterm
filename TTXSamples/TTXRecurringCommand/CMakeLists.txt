set(PACKAGE_NAME "TTXRecurringCommand")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME} SHARED
  TTXRecurringCommand.c
  TTXRecurringCommand.rc
  ReadMe.txt
  ReadMe-ja.txt
  ../../teraterm/common/dlglib.h
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif()

source_group(
  "common"
  REGULAR_EXPRESSION
  "teraterm\/common\/"
  )

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../../teraterm/teraterm
  ../../teraterm/common
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  ttpcmn
  #
  gdi32
  )

install(
  TARGETS ${PACKAGE_NAME}
  RUNTIME
  DESTINATION .
  )
