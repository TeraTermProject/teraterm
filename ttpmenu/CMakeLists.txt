project("ttpmenu")

include(${CMAKE_CURRENT_SOURCE_DIR}/../libs/lib_libressl.cmake)

add_executable(
  ${PROJECT_NAME} WIN32
  registry.cpp
  registry.h
  resource.h
  ttpmenu-version.h
  ttpmenu-version.rc
  ttpmenu.cpp
  ttpmenu.h
  winmisc.cpp
  #
  left.ico
  right.ico
  teraterm.ico
  ttpmenu.rc
  )

target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE
#  UNICODE=1 _UNICODE=1
)

if(MSVC)
  target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    /W4
  )
endif(MSVC)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
  ../teraterm/common/ttlib.h
  ../teraterm/common/i18n.h
  ../teraterm/common/codeconv.h
  ../teraterm/common/compat_win.h
  ../teraterm/common/dllutil.h
)

source_group(
  "common"
  REGULAR_EXPRESSION
  "teraterm\/common\/")

target_sources(
  ${PROJECT_NAME}
  PRIVATE
  ${LIBRESSL_INCLUDE_DIRS}/openssl/sha.h
  ${LIBRESSL_INCLUDE_DIRS}/openssl/evp.h
  ${LIBRESSL_INCLUDE_DIRS}/openssl/rand.h
  ${LIBRESSL_INCLUDE_DIRS}/openssl/hmac.h
  ${LIBRESSL_INCLUDE_DIRS}/openssl/crypto.h
)

source_group(
  "libressl"
  REGULAR_EXPRESSION
  "${LIBRESSL_INCLUDE_DIRS}/openssl/")

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    layer_for_unicode
    )
  if(MSVC)
    target_sources(
      ${PROJECT_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  .
  ${LIBRESSL_INCLUDE_DIRS}
  )

if(MSVC)
  if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
  else()
    target_link_options(
      ${PROJECT_NAME}
      PRIVATE
      /MANIFEST:NO)
  endif()
endif()

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  common_static
  comctl32
  user32
  gdi32
  comdlg32
  ${LIBRESSL_LIB}
  bcrypt
  ws2_32
  )

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION .
  )
