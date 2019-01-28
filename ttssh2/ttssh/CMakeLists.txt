project(ttssh)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(SRC
  resource.h
  ttsecure.ico
  ttssh.c
  ttssh.rc
  )

add_executable(
  ttssh WIN32
  ${SRC}
  )
