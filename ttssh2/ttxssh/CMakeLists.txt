project(ttxssh)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_zlib.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/lib_openssl.cmake)

if(USE_UNICODE_API)
  add_definitions(-DUNICODE -D_UNICODE)
endif()

set(COMMON_SRC
  ../../teraterm/teraterm/WSAAsyncGetAddrInfo.c
  ../../teraterm/common/ttcommon.h
  ../../teraterm/common/i18n.h
  ../../teraterm/common/ttlib.h
  ../../teraterm/common/dlglib.h
  ../../teraterm/common/servicenames.h
  ../../teraterm/common/codeconv.cpp
  ../../teraterm/common/codeconv.h
  )

source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(SRC
  ${CMAKE_CURRENT_BINARY_DIR}/puttyversion.h
  ../matcher/matcher.c
  arc4random.c
  arc4random.h
  auth.c
  auth.h
  buffer.c
  buffer.h
  chacha.c
  chacha.h
  cipher.h
  cipher-ctr.c
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
  fwd.c
  fwd.h
  fwd-socks.c
  fwd-socks.h
  fwdui.c
  fwdui.h
  hosts.c
  hosts.h
  kex.c
  kex.h
  key.c
  key.h
  keyfiles.c
  keyfiles.h
  pkt.c
  pkt.h
  resource.h
  sftp.c
  sftp.h
  ssh.c
  ssh.h
  ttxssh.c
  ttxssh.def
  ttxssh.h
  ttxssh.rc
  ttxssh-version.h
  ttxssh-version.rc
  util.c
  util.h
  x11util.c
  x11util.h
  ${COMMON_SRC}
  )


include_directories(
  ../../teraterm/teraterm
  ../../teraterm/common
  ../matcher
  ../putty
  ${OPENSSL_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
  )

link_directories(
  ${ZLIB_LIBRARY_DIRS}
  )

if (MSVC)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DELAYLOAD:dnsapi.dll")
endif()

add_library(
  ttxssh SHARED
  ${SRC}
  )

target_link_libraries(
  ttxssh
  libputty
  ttpcmn
  debug zlibstaticd
  optimized zlibstatic
  ${OPENSSL_LIB}
  #
  ws2_32
  dnsapi
  gdi32
  comdlg32
  delayimp
  )

### create puttyversion.h

file(READ "${CMAKE_SOURCE_DIR}/libs/putty/windows/putty.iss" iis)

string(REGEX MATCH "VersionInfoTextVersion=([^\n]+)\n" _ ${iis})
set(version_str ${CMAKE_MATCH_1})
if("${version_str}" STREQUAL "")
  string(REGEX MATCH "AppVerName=([^\n]+)\n" _ ${iis})
  set(version_str ${CMAKE_MATCH_1})
endif()
if("${version_str}" STREQUAL "")
  set(version_str "unknown")
endif()
message("putty version=\"${version_str}\"")
FILE(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/puttyversion.h
  "#define PUTTYVERSION \"${version_str}\"\n"
  )
