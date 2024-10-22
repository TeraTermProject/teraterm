set(PACKAGE_NAME "cyglaunch")

project(${PACKAGE_NAME})

add_executable(
  ${PACKAGE_NAME} WIN32
  cyglaunch.c
  cyglaunch.rc
  ../cyglib/cyglib.h
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  .
  )

if(SUPPORT_OLD_WINDOWS)
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
      -Wl,--no-whole-archive
    )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  cyglib
  )

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )

#install(
#  FILES
#  cyglaunch_cmd.bat
#  cyglaunch_cmd_msys2term.bat
#  cyglaunch_git_bash.bat
#  cyglaunch_msys2_mingw32.bat
#  cyglaunch_msys2_mingw64.bat
#  cyglaunch_msys2_msys.bat
#  cyglaunch_powershell.bat
#  cyglaunch_powershell_msys2term.bat
#  cyglaunch_wsl.bat
#  cyglaunch_wsl_msys2term.bat
#  DESTINATION .
#)

if(MSVC)
  # subsystem:windows, but start form wmain()
  if(${CMAKE_VERSION} VERSION_LESS "3.13.0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:wmainCRTStartup")
  else()
    target_link_options(
      ${PACKAGE_NAME}
      PRIVATE
      /ENTRY:wmainCRTStartup
      )
  endif()
endif(MSVC)

if(MINGW)
  target_link_options(
    ${PACKAGE_NAME}
    PRIVATE
    -municode
    )
endif(MINGW)
