
add_custom_target(
  cygtool_build ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  )

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygtool_build)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  COMMAND ${CMAKE_COMMAND} -DCMAKE_GENERATOR=${CMAKE_GENERATOR} -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/$<CONFIG> -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -P ${CMAKE_CURRENT_LIST_DIR}/../cygtool/build_cygtool.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/build_mark.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygtool_build
  COMMENT Generate "${CMAKE_CURRENT_BINARY_DIR}/cygtool.dll"
  )

install(
  FILES ${CMAKE_BINARY_DIR}/$<CONFIG>/cygtool.dll
  DESTINATION .
  )
