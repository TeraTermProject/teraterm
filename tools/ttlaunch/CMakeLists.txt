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

if(MSVC)
  target_compile_options(${PROJECT_NAME}
    PRIVATE
    /W4
    #/U_CRT_SECURE_NO_DEPRECATE
    #/sdl
    $<$<CONFIG:Debug>:/GS>)
endif()

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

if(MSVC)
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
    /ENTRY:wmainCRTStartup
    /MANIFEST:NO
  )
endif(MSVC)

if(MINGW)
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
    -municode
  )
endif(MINGW)

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
#  GetPrivateProfileString() で読めるのは ANSI か UTF-16LE
#  UTF-16LEに変換したものを出力
#  出力フォルダ(BINARY_DIR)は、"${CMAKE_BINARY_DIR}/$<CONFIG>" or "${CMAKE_BINARY_DIR}"
#  ANSI版は未対応
if(NOT DEFINED BINARY_DIR)
  message(FATAL_ERROR "check BINARY_DIR")
endif()
set(LNG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ttlaunch.lng)
set(LNG_DEST ${BINARY_DIR}/ttlaunch.lng)
set(LNG_SCRIPT ${CMAKE_SOURCE_DIR}/installer/utf8_to.pl)
add_custom_command(
  OUTPUT ${LNG_DEST}
  DEPENDS ${LNG_SRC} ${LNG_SCRIPT}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${BINARY_DIR}
  COMMAND ${PERL} ${LNG_SCRIPT} utf16le ${LNG_SRC} ${LNG_DEST}
)
add_custom_target(
  ${PROJECT_NAME}_lng
  DEPENDS ${LNG_DEST}
  SOURCES ${LNG_SRC}
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_lng)
install(
  FILES ${LNG_DEST}
  DESTINATION .
)
