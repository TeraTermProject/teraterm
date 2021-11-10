set(PACKAGE_NAME "cyglaunch")

project(${PACKAGE_NAME})

add_executable(
  ${PACKAGE_NAME} WIN32
  cyglaunch.c
  cyglaunch.rc
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  .
  )

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )

if(MSVC)
  # subsystem:windows, but start form main()
  if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
  else()
    target_link_options(
      ${PACKAGE_NAME}
      PRIVATE
      /ENTRY:wmainCRTStartup
      )
  endif()
endif(MSVC)
