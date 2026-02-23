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
  bcrypt_pbkdf.c
  blf.h
  blowfish.c
  buffer.c
  buffer.h
  chacha.c
  chacha.h
  cipher-3des1.c
  cipher-3des1.h
  cipher-chachapoly-libcrypto.c
  cipher-chachapoly.h
  cipher-ctr.c
  cipher-ctr.h
  cipher.c
  cipher.h
  comp.c
  comp.h
  config.h
  crypt.c
  crypt.h
  crypto_api.h
  digest.c
  digest.h
  dns.c
  dns.h
  ed25519.c
  fwd-socks.c
  fwd-socks.h
  fwd.c
  fwd.h
  fwdui.c
  fwdui.h
  hash.c
  hmac.c
  hmac.h
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
  openbsd-compat.c
  openbsd-compat.h
  pkt.c
  pkt.h
  poly1305.c
  poly1305.h
  resource.h
  sftp.c
  sftp.h
  smult_curve25519_ref.c
  sntrup761.c
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
  win32helper_u8.cpp
  win32helper_u8.h
  x11util.c
  x11util.h
)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../../teraterm/teraterm/WSAAsyncGetAddrInfo.c
  ../../teraterm/common/ttcommon.h
  ../../teraterm/common/i18n.h
  ../../teraterm/common/ttlib.h
  ../../teraterm/common/dlglib.h
  ../../teraterm/common/servicenames.h
  ../../teraterm/common/codeconv.h
  ../../teraterm/common/tipwin.h
  ../../teraterm/common/compat_win.h
  ../../teraterm/common/dllutil.h
  )

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../../teraterm/common/edithistory.cpp
  ../../teraterm/common/edithistory.h
  ../../teraterm/common/edithistory.rc
  ../../teraterm/common/edithistory_res.h
)

source_group(
  "common"
  REGULAR_EXPRESSION
  "teraterm\/common\/")

if(MINGW)
  target_compile_options(
    ${PACKAGE_NAME}
    PRIVATE
    -Wno-error=incompatible-pointer-types
  )
endif(MINGW)

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
    target_link_libraries(
      ${PACKAGE_NAME}
      PRIVATE
      -Wl,--whole-archive
      mingw_msvcrt
      mingw_dll_tls
      -Wl,--no-whole-archive
    )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../../teraterm/teraterm
  ../matcher
  ../libsshagentc
  ../../libs/include
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
  libsshagentc
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
