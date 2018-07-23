project(keycode)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  keycode.c
  keycode.ico
  keycode-version.rc
  keycode.rc
  kc_res.h
  )


include_directories(
  ../common
  ../ttpfile
  .
  )
add_executable(
  keycode WIN32
  ${SRC}
  )
target_link_libraries(
  keycode
  )


