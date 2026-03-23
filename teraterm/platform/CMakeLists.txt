set(PACKAGE_NAME "platform")

project(${PACKAGE_NAME})

if(APPLE)
  # macOS platform layer
  enable_language(OBJC)

  add_library(
    ${PACKAGE_NAME}
    STATIC
    platform_def.h
    platform_macos_types.h
    platform_macos_compat.c
    platform_stubs.c
    macos/macos_window.h
    macos/macos_window.m
    macos/macos_serial.h
    macos/macos_serial.c
    macos/macos_net.h
    macos/macos_net.c
    macos/macos_pty.h
    macos/macos_pty.c
  )

  target_include_directories(
    ${PACKAGE_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/macos
  )

  target_link_libraries(
    ${PACKAGE_NAME}
    PUBLIC
    "-framework Cocoa"
    "-framework AppKit"
    "-framework CoreGraphics"
    "-framework CoreText"
    "-framework CoreFoundation"
    "-framework IOKit"
    "-framework Security"
  )

  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE
    -fobjc-arc
  )

  # Set Objective-C ARC for .m files
  set_source_files_properties(
    macos/macos_window.m
    PROPERTIES
    COMPILE_FLAGS "-fobjc-arc"
  )

elseif(WIN32)
  # On Windows, this is a no-op placeholder
  add_library(
    ${PACKAGE_NAME}
    STATIC
    platform_def.h
  )
  # Set a dummy source to avoid empty library error
  set_target_properties(${PACKAGE_NAME} PROPERTIES LINKER_LANGUAGE C)

  target_include_directories(
    ${PACKAGE_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()
