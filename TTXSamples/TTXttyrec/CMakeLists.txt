project(TTXttyrec)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  )

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
  #
  comdlg32
  )

add_library(
  TTXttyrec SHARED
  TTXttyrec.c
  gettimeofday.c
  gettimeofday.h
  ReadMe.txt
  ReadMe-ja.txt
  )

target_link_libraries(
  TTXttyrec
  ttpcmn
  comdlg32
  )

set_target_properties(
  TTXttyrec TTXttyplay
  PROPERTIES
  PREFIX ""
  )
