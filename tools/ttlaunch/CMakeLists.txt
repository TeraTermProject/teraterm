project("ttlaunch")

add_executable(
  ${PROJECT_NAME} WIN32
  ttlaunch.cpp
  ttlaunch.rc
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

if(MSVC)
  # subsystem:windows, but start form wmain()
  if(${CMAKE_VERSION} VERSION_LESS "3.13.0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:wmainCRTStartup")
  else()
    target_link_options(
      ${PROJECT_NAME}
      PRIVATE
      /ENTRY:wmainCRTStartup
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
