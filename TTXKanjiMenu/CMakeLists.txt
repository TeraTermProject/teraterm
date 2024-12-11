set(PACKAGE_NAME "ttxkanjimenu")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME} SHARED
  resource.h
  ttxkanjimenu.c
  ttxkanjimenu.h
  ttxkanjimenu.rc
  )

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PACKAGE_NAME}
    PRIVATE
    layer_for_unicode
    )
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_link_libraries(
      ${PACKAGE_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      mingw_dll_tls
      -Wl,--no-whole-archive
    )
  endif()
endif(SUPPORT_OLD_WINDOWS)

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../teraterm/teraterm
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  ttpcmn
  common_static
  )

install(
  TARGETS ${PACKAGE_NAME}
  RUNTIME
  DESTINATION .
  )
