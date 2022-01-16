set(PACKAGE_NAME "ttxssh")

option(TTXSSH_LIBRESSL "User LibreSSL" on)
option(TTXSSH_OPENSSL3 "Use OpenSSL3" off)

project(${PACKAGE_NAME})

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_zlib.cmake)
if(TTXSSH_LIBRESSL)
  include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_libressl.cmake)
elseif(TTXSSH_OPENSSL3)
  include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_openssl3.cmake)
else()
  include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_openssl.cmake)
endif()

add_library(
  ${PACKAGE_NAME} SHARED
  )

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../matcher/matcher.c
  arc4random.c
  arc4random.h
  auth.c
  auth.h
  buffer.c
  buffer.h
  chacha.c
  chacha.h
  cipher-3des1.c
  cipher-chachapoly-libcrypto.c
  cipher-chachapoly.h
  cipher-ctr.c
  cipher.c
  cipher.h
  comp.c
  comp.h
  config.h
  crypt.c
  crypt.h
  dns.c
  dns.h
  ed25519.c
  ed25519_bcrypt_pbkdf.c
  ed25519_blf.h
  ed25519_blocks.c
  ed25519_blowfish.c
  ed25519_crypto_api.h
  ed25519_fe25519.c
  ed25519_fe25519.h
  ed25519_ge25519.c
  ed25519_ge25519.h
  ed25519_hash.c
  ed25519_sc25519.c
  ed25519_sc25519.h
  ed25519_verify.c
  fwd-socks.c
  fwd-socks.h
  fwd.c
  fwd.h
  fwdui.c
  fwdui.h
  hostkey.c
  hostkey.h
  hosts.c
  hosts.h
  kex.c
  kex.h
  key.c
  key.h
  keyfiles.c
  keyfiles.h
  keyfiles-putty.c
  keyfiles-putty.h
  mac.c
  mac.h
  pkt.c
  pkt.h
  poly1305.c
  poly1305.h
  resource.h
  sftp.c
  sftp.h
  ssh.c
  ssh.h
  ssherr.c
  ssherr.h
  ttxssh-version.h
  ttxssh-version.rc
  ttxssh.c
  ttxssh.def
  ttxssh.h
  ttxssh.rc
  util.c
  util.h
  x11util.c
  x11util.h
  ${COMMON_SRC}
  )

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../../teraterm/teraterm/WSAAsyncGetAddrInfo.c
  ../../teraterm/common/ttcommon.h
  ../../teraterm/common/i18n.h
  ../../teraterm/common/ttlib.h
  ../../teraterm/common/ttlib.c
  ../../teraterm/common/dlglib.h
  ../../teraterm/common/servicenames.h
  ../../teraterm/common/codeconv.h
  ../../teraterm/common/tipwin.h
  ../../teraterm/common/compat_win.h
  ../../teraterm/common/dllutil.h
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "teraterm\/common\/")

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
      set(CMAKE_C_STANDARD_LIBRARIES "")
      set(CMAKE_CXX_STANDARD_LIBRARIES "")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
    else()
      target_link_options(
        ${PACKAGE_NAME}
        PRIVATE
        /NODEFAULTLIB
        )
    endif()
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
  if(MINGW)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/libmingw/tlssup.c
      )
    target_link_libraries(
      ${PACKAGE_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      -Wl,--no-whole-archive
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../../teraterm/teraterm
  ../../teraterm/common
  ../matcher
  ../putty
  ${ZLIB_INCLUDE_DIRS}
  )

if(TTXSSH_LIBRESSL)
  target_include_directories(
    ${PACKAGE_NAME}
    PRIVATE
    ${LIBRESSL_INCLUDE_DIRS}
    )
elseif(TTXSSH_OPENSSL3)
  target_include_directories(
    ${PACKAGE_NAME}
    PRIVATE
    ${OPENSSL3_INCLUDE_DIRS}
    )
else()
  target_include_directories(
    ${PACKAGE_NAME}
    PRIVATE
    ${OPENSSL_INCLUDE_DIRS}
    )
endif()


set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_compile_definitions(
  ${PACKAGE_NAME}
  PRIVATE
  $<$<CONFIG:Debug>:_CRTDBG_MAP_ALLOC>
  )

if(MINGW)
  target_link_options(
    ttxssh
    PRIVATE
    -Wl,-Map=ttxssh.map,-cref
    )
endif()

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  libputty
  ttpcmn
  argon2
  ${ZLIB_LIB}
  )

if(TTXSSH_LIBRESSL)
  target_link_libraries(
    ${PACKAGE_NAME}
    PRIVATE
    ${LIBRESSL_LIB}
    bcrypt.lib
    )
elseif(TTXSSH_OPENSSL3)
  target_link_libraries(
    ${PACKAGE_NAME}
    PRIVATE
    ${OPENSSL3_LIB}
    bcrypt.lib
    )
else()
  target_link_libraries(
    ${PACKAGE_NAME}
    PRIVATE
    ${OPENSSL_LIB}
    crypt32.lib		# TODO このlibを使用しないパッチを適応したら削除すること
    )
endif()

if(SUPPORT_OLD_WINDOWS)
  target_link_libraries(
    ${PACKAGE_NAME}
    PRIVATE
    layer_for_unicode
    )
endif()

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  ws2_32
  gdi32
  comdlg32
  comctl32
  )

install(
  TARGETS ttxssh
  RUNTIME
  DESTINATION .
  )
