set(PACKAGE_NAME "argon2")

project(${PACKAGE_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

add_library(
  ${PACKAGE_NAME}
  argon2.h
  ../../libs/argon2/src/argon2.c
  ../../libs/argon2/src/blake2/blake2-impl.h
  ../../libs/argon2/src/blake2/blake2.h
  ../../libs/argon2/src/blake2/blake2b.c
  ../../libs/argon2/src/blake2/blamka-round-opt.h
  ../../libs/argon2/src/blake2/blamka-round-ref.h
  ../../libs/argon2/src/core.c
  ../../libs/argon2/src/core.h
  ../../libs/argon2/src/encoding.c
  ../../libs/argon2/src/encoding.h
  ../../libs/argon2/src/ref.c
  ../../libs/argon2/src/thread.c
  ../../libs/argon2/src/thread.h
  )

if(MSVC AND (MSVC_VERSION LESS_EQUAL 1500))
  # Visual Studio 2005, 2008  (1500=VS2008)
  target_sources(
    ${PACKAGE_NAME}
    PRIVATE
    compat/stdint.h
    )
  target_include_directories(
    ${PACKAGE_NAME}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/compat
    )
endif()

source_group(
  "libs_argon2"
  REGULAR_EXPRESSION
  "libs/argon2/")

target_include_directories(
  ${PACKAGE_NAME}
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../libs/argon2/src
  )

