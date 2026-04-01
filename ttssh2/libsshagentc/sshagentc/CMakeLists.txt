project("libsshagentc")

add_library(
  ${PROJECT_NAME}
  ../libputty.h
  agentc.cpp
  sha256.h
)

if(1)
  target_sources(
    ${PROJECT_NAME}
    PRIVATE
    sha256.h
    sha256_bcrypt.cpp
  )
endif()

if(0)
  target_sources(
    ${PROJECT_NAME}
    PRIVATE
    sha256.h
    sha256_openssl.cpp
  )
endif()

if(MSVC)
  target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    -W4
  )
else()
  target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    -Wall -Wextra
  )
endif()

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  crypt32
)
