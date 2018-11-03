project(TTXttyrec)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

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
  TTXttyplay SHARED
  TTXttyplay.c
  gettimeofday.c
  gettimeofday.h
  ReadMe.txt
  ReadMe-ja.txt
  )

target_link_libraries(
  TTXttyplay
  ttpcmn
  )

add_library(
  TTXttyrec SHARED
  TTXttyrec.c
  gettimeofday.c
  gettimeofday.h
  ReadMe.txt
  ReadMe-ja.txt
  )

set_target_properties(
  TTXttyrec TTXttyplay
  PROPERTIES
  PREFIX ""
  )
