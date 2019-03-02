project(TTXResizeMenu)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXResizeMenu.c
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

if (MSVC)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:user32.dll")
endif()

add_library(
  TTXResizeMenu SHARED
  ${SRC}
  )

target_link_libraries(
  TTXResizeMenu
  ttpcmn
  delayimp
  )


