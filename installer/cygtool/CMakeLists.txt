add_library(
  cygtool_dll SHARED
  cygtool.c
  cygtool.def
  )

target_compile_options(
  cygtool_dll
  PRIVATE
  -I ${CMAKE_CURRENT_LIST_DIR}/../../teraterm/common
  )

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
