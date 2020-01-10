project(keycode)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  keycode WIN32
  keycode.c
  keycode.ico
  keycode-version.rc
  keycode.rc
  kc_res.h
  )

target_include_directories(
  keycode
  PRIVATE
  ../common
  )

if(MSVC)
  target_compile_options(
    keycode
    PRIVATE
    /W4
    )
endif()

target_compile_options(
  keycode
  PRIVATE
  )

install(
  TARGETS keycode
  DESTINATION .
  )
