cmake_minimum_required(VERSION 3.11)

option(UNICODE "use Unicode Win32 API" ON)

message("CMAKE_COMMAND=${CMAKE_COMMAND}")
message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
message("CMAKE_HOST_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME}")
message("UNICODE=${UNICODE}")
if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "MSYS")
  message("MSYS2TERM=ON")
  set(MSYS2TERM ON)
endif()

if (MSYS2TERM)
  set(PACKAGE_NAME "msys2term")
else()
  set(PACKAGE_NAME "cygterm")
endif()

project(${PACKAGE_NAME})
ENABLE_LANGUAGE(RC)

file(WRITE "${CMAKE_CURRENT_LIST_DIR}/cygterm.rc" "icon ICON cygterm.ico")

add_executable(
  ${PACKAGE_NAME}
  cygterm.cpp
  cygterm_cfg.cpp
  cygterm_cfg.h
  sub.cpp
  sub.h
  ${CMAKE_CURRENT_LIST_DIR}/cygterm.rc
  )

if (MSYS2TERM)
  target_compile_definitions(
    ${PACKAGE_NAME}
    PRIVATE
    MSYS2=1
    )
else()
  target_compile_definitions(
    ${PACKAGE_NAME}
    PRIVATE
    CYGWIN=1
    )
endif()

target_compile_options(
  ${PACKAGE_NAME}
  PRIVATE
  -D_GNU_SOURCE
  -fno-exceptions
  -Wall -Wextra
  )

if(UNICODE)
  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE
    -DUNICODE=1
    -D_UNICODE=1
    )
endif()

target_link_options(
  ${PACKAGE_NAME}
  PRIVATE
  -mwindows
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  shell32
  ole32
  )

install(
  TARGETS ${PACKAGE_NAME}
  DESTINATION .
  )
install(
  FILES ${PACKAGE_NAME}.cfg
  DESTINATION .
  )


set(ARCHIVE "cygterm+.tar.gz")

set(SRC
  cygterm.cpp
  cygterm_cfg.cpp
  cygterm_cfg.h
  sub.cpp
  sub.h
  #
  cygterm.ico
  #
  cygterm.cfg
  msys2term.cfg
  )

add_custom_target(
  tar ALL
  SOURCES ${CMAKE_CURRENT_LIST_DIR}/${ARCHIVE}
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_LIST_DIR}/${ARCHIVE}
  DEPENDS ${SRC}
  COMMAND ${CMAKE_COMMAND} -E tar cvz ${CMAKE_CURRENT_LIST_DIR}/${ARCHIVE} COPYING README README-j Makefile CMakeLists.txt ${SRC}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
