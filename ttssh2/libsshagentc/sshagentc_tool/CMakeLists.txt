project("sshagentc_tool")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ${PROJECT_NAME}
  main.cpp
  ../libputty.h
)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  .
  ../
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  libsshagentc
)

if(MINGW)
  target_link_options(
    ${PROJECT_NAME}
    PRIVATE
    -municode
  )
endif()
