set(PACKAGE_NAME "libsshagentc")

project(${PACKAGE_NAME})

add_library(
  ${PACKAGE_NAME}
  ../libputty.h
  agentc.cpp
  sha256.h
)

if(1)
  target_sources(
    ${PACKAGE_NAME}
    PRIVATE
    sha256.h
    sha256_bcrypt.cpp
  )
endif()

if(0)
  target_sources(
    ${PACKAGE_NAME}
    PRIVATE
    sha256.h
    sha256_openssl.cpp
  )
endif()

if(MSVC)
  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE
    -W4
  )
else()
  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE
    -Wall -Wextra
  )
endif()

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  crypt32
)
