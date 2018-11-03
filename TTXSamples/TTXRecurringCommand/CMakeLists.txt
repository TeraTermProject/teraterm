project(TTXRecurringCommand)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  TTXRecurringCommand.c
  TTXRecurringCommand.rc
  ReadMe.txt
  ReadMe-ja.txt
  )

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

if(MINGW)
  link_directories(
	${CMAKE_SOURCE_DIR}/teraterm/htmlhelp
	)
endif()

add_library(
  TTXRecurringCommand SHARED
  ${SRC}
  )

set_target_properties(
  TTXRecurringCommand
  PROPERTIES
  PREFIX ""
  )

target_link_libraries(
  TTXRecurringCommand
  ttpcmn
  #
  gdi32
  )
