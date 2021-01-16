set(PACKAGE_NAME "cygtool_dll")

add_library(
  ${PACKAGE_NAME} SHARED
  cygtool.c
  cygtool.def
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

target_link_libraries(
  cygtool_dll
  PRIVATE
  version
  )

set_target_properties(
  cygtool_dll
  PROPERTIES
  OUTPUT_NAME "cygtool"
  )

set_target_properties(
  cygtool_dll
  PROPERTIES
  FOLDER installer
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
  )

install(
  TARGETS cygtool_dll
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
    cygtool_dll
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
