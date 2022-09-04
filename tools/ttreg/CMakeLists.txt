set(PACKAGE_NAME "ttreg")

project(${PACKAGE_NAME})

add_executable(
  ${PACKAGE_NAME}
  ttreg.cpp
)

if(MSVC)
  if(${CMAKE_VERSION} VERSION_LESS "3.13.0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:wmainCRTStartup")
  else()
    target_link_options(
      ${PACKAGE_NAME}
      PRIVATE
      /ENTRY:wmainCRTStartup
    )
  endif()
else(MSVC)
  target_link_options(
    ${PACKAGE_NAME}
    PRIVATE
    -municode
  )
endif(MSVC)

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
)
