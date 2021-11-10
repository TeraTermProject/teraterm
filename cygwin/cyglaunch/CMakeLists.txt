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

if(MSVC)
  # subsystem:windows, but start form wmain()
  if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
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
