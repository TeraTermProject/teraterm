project("ttlaunch")

add_executable(
  ${PROJECT_NAME} WIN32
  ttlaunch.cpp
  ttlaunch.h
  ttlaunch_setup.cpp
  ttlaunch_setup.h
  ttlaunch_version.h
  ttlaunch.rc
  ttlaunch_manifest.rc
  resource.h
  ../../cygwin/cyglib/cyglib.h
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
    )
  endif()
  if(MINGW)
    target_link_libraries(
      ${PROJECT_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      -Wl,--no-whole-archive
    )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  common_static
  cyglib
  )

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION .
  )
install(
  FILES README.md
  DESTINATION .
  RENAME ttlaunch.md
  )

# ttlaunch.lng (UTF-8) -> UTF-16LE
#  GetPrivateProfileString() で読めるのは ANSI か UTF-16LE のため
set(LNG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ttlaunch.lng)
set(LNG_DEST ${CMAKE_CURRENT_BINARY_DIR}/lng/ttlaunch.lng)
set(LNG_SCRIPT ${CMAKE_SOURCE_DIR}/installer/utf8_to.pl)
add_custom_command(
  OUTPUT ${LNG_DEST}
  DEPENDS ${LNG_SRC} ${LNG_SCRIPT}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/lng
  COMMAND ${PERL} ${LNG_SCRIPT} utf16le ${LNG_SRC} ${LNG_DEST}
  )
add_custom_target(
  ttlaunch_lng
  DEPENDS ${LNG_DEST}
  SOURCES ${LNG_SRC}
  )
add_dependencies(${PROJECT_NAME} ttlaunch_lng)
# 実行ファイルと同じフォルダに置く (ビルドツリーから実行するとき用)
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LNG_DEST} $<TARGET_FILE_DIR:${PROJECT_NAME}>/ttlaunch.lng
  )
install(
  FILES ${LNG_DEST}
  DESTINATION .
  )

if(MSVC)
  # subsystem:windows, but start form wmain()
  if(${CMAKE_VERSION} VERSION_LESS "3.13.0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:wmainCRTStartup /MANIFEST:NO")
  else()
    target_link_options(
      ${PROJECT_NAME}
      PRIVATE
      /ENTRY:wmainCRTStartup
      /MANIFEST:NO
      )
  endif()
endif(MSVC)

if(MINGW)
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
    -municode
    )
endif(MINGW)
