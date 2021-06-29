set(PACKAGE_NAME "keycode")

project(${PACKAGE_NAME})

add_executable(
  ${PACKAGE_NAME} WIN32
  keycode.c
  keycode.ico
  keycode-version.rc
  keycode.rc
  kc_res.h
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
endif(SUPPORT_OLD_WINDOWS)

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../common
  )

if(MSVC)
  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE
    /W4
    )
endif()

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )

### unicode version

set(PACKAGE_NAME_2 "keycodeW")

add_executable(
  ${PACKAGE_NAME_2} WIN32
  keycode.c
  keycode.ico
  keycode-version.rc
  keycode.rc
  kc_res.h
  )

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PACKAGE_NAME_2}
    PRIVATE
    common_static
    layer_for_unicode
    )
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME_2}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

set_target_properties(
  ${PACKAGE_NAME_2}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_compile_definitions(
  ${PACKAGE_NAME_2}
  PRIVATE
  UNICODE _UNICODE
  )

target_include_directories(
  ${PACKAGE_NAME_2}
  PRIVATE
  ../common
  )

if(MSVC)
  target_compile_options(
    ${PACKAGE_NAME_2}
    PRIVATE
    /W4
    )
endif()
