project(ttxssh)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_zlib.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_openssl.cmake)

if (MSVC)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DELAYLOAD:user32.dll /DELAYLOAD:dnsapi.dll")
endif()

add_library(
  ttxssh SHARED
  )

target_sources(
  ttxssh
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
  ttxssh
  PRIVATE
  ../../teraterm/teraterm/WSAAsyncGetAddrInfo.c
  ../../teraterm/common/ttcommon.h
  ../../teraterm/common/i18n.h
  ../../teraterm/common/ttlib.h
  ../../teraterm/common/ttlib.c
  ../../teraterm/common/dlglib.h
  ../../teraterm/common/dlglib.c
  ../../teraterm/common/dlglib_cpp.cpp
  ../../teraterm/common/dlglib_tmpl.cpp
  ../../teraterm/common/servicenames.h
  ../../teraterm/common/codeconv.cpp
  ../../teraterm/common/codeconv.h
  ../../teraterm/common/tipwin.cpp
  ../../teraterm/common/tipwin.h
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "teraterm\/common\/")

target_include_directories(
  ttxssh
  PRIVATE
  ../../teraterm/teraterm
  ../../teraterm/common
  ../matcher
  ../putty
  ${OPENSSL_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
)

set_target_properties(
  ttxssh
  PROPERTIES
  PREFIX ""
  )

target_compile_definitions(
  ttxssh
  PRIVATE
  $<$<CONFIG:Debug>:_CRTDBG_MAP_ALLOC>
  )
if(USE_UNICODE_API)
  target_compile_definitions(
    ttxssh
    PRIVATE
    UNICODE
    _UNICODE
    )
endif()


target_link_libraries(
  ttxssh
  libputty
  ttpcmn
  argon2
  ${ZLIB_LIB}
  ${OPENSSL_LIB}
  #
  ws2_32
  dnsapi
  gdi32
  comdlg32
  delayimp
  # OpenSSL が使用
  crypt32.lib		# TODO このlibを使用しないパッチを適応したら削除すること
  )

install(
  TARGETS ttxssh
  RUNTIME
  DESTINATION .
  )
