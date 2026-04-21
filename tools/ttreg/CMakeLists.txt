project("ttreg")

add_executable(
  ${PROJECT_NAME}
  ttreg.cpp
)

if(MSVC)
  if(${CMAKE_VERSION} VERSION_LESS "3.13.0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:wmainCRTStartup")
  else()
    target_link_options(
      ${PROJECT_NAME}
      PRIVATE
      /ENTRY:wmainCRTStartup
    )
  endif()
else(MSVC)
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
    -municode
  )
endif(MSVC)

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION .
)
install(FILES
  README.md
  DESTINATION .
  RENAME ttreg.md)
