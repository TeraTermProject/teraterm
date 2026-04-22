project("keycode")

add_executable(
  ${PROJECT_NAME} WIN32
  keycode.c
  keycode.ico
  keycode-version.rc
  keycode.rc
  kc_res.h
  )

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    layer_for_unicode
    )
  if(MSVC)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  common_static
  ttpcmn
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

if(MSVC)
  target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    /W4
    )
endif()

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION .
  )

### unicode version

project("keycodeW")

add_executable(
  ${PROJECT_NAME} WIN32
  keycode.c
  keycode.ico
  keycode-version.rc
  keycode.rc
  kc_res.h
  )

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    layer_for_unicode
    )
  if(MSVC)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  common_static
  ttpcmn
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE
  UNICODE _UNICODE
  )

if(MSVC)
  target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    /W4
    )
endif()
