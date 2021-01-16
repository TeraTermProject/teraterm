project(TTXttyrec)

add_library(
  TTXttyplay SHARED
  TTXttyplay.c
  gettimeofday.c
  gettimeofday.h
  ReadMe.txt
  ReadMe-ja.txt
  )

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      TTXttyplay
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_include_directories(
  TTXttyplay
  PRIVATE
  ../../teraterm/teraterm
  ../../teraterm/common
  )

target_link_libraries(
  TTXttyplay
  PRIVATE
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

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      TTXttyrec
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_include_directories(
  TTXttyrec
  PRIVATE
  ../../teraterm/teraterm
  ../../teraterm/common
  )

target_link_libraries(
  TTXttyrec
  PRIVATE
  ttpcmn
  comdlg32
  )

set_target_properties(
  TTXttyrec TTXttyplay
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

install(
  TARGETS TTXttyrec TTXttyplay
  RUNTIME
  DESTINATION .
  )
