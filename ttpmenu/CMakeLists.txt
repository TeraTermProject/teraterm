set(PACKAGE_NAME "ttpmenu")

project(${PACKAGE_NAME})

add_executable(
  ${PACKAGE_NAME} WIN32
  resource.h
  registry.cpp
  ttpmenu.h
  ttpmenu.cpp
  winmisc.cpp
  #
  left.ico
  right.ico
  teraterm.ico
  ttpmenu.rc
  )

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../teraterm/common/ttlib.h
  ../teraterm/common/ttlib.c
  ../teraterm/common/i18n.h
  ../teraterm/common/i18n.c
  ../teraterm/common/codeconv.h
  ../teraterm/common/compat_win.h
  ../teraterm/common/dllutil.h
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
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

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../teraterm/common
  .
  )


target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  version
  comctl32
  user32
  gdi32
  comdlg32
  )

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )
