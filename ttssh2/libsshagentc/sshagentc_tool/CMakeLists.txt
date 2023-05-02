set(PACKAGE_NAME "sshagentc_tool")

project(${PACKAGE_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_executable(
  ${PACKAGE_NAME}
  main.cpp
  ../libputty.h
)

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  .
  ../
)

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  libsshagentc
)

if(MINGW)
  target_link_options(
    ${PACKAGE_NAME}
    PRIVATE
    -municode
  )
endif()
