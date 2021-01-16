set(PACKAGE_NAME "ttptek")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME} SHARED
  tekesc.c
  tekesc.h
  ttptek.def
  ttptek-version.rc
  tttek.h
  tttek.c
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../common/tttypes.h
  ../common/i18n.h
  ../common/ttlib.c
  ../common/ttlib.h
  ../common/codeconv.h
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "..\/common\/"
  )

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  )

if(MINGW)
  set_target_properties(
    ${PACKAGE_NAME}
    PROPERTIES PREFIX ""
    SUFFIX ".dll"
    )
endif()

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  ttpcmn
  #
  iphlpapi
  gdi32
  )

install(
  TARGETS ${PACKAGE_NAME}
  RUNTIME
  DESTINATION .
  )
