project(ttproxy)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

include(${CMAKE_CURRENT_SOURCE_DIR}/../libs/lib_openssl.cmake)

set(SRC
  ../teraterm/common/ttlib.c
  ../teraterm/common/ttlib.h
  ../teraterm/common/i18n.c
  ../teraterm/common/i18n.h

  Hooker.h
  Logger.h
  ProxyWSockHook.h
  resource.h
  SSLLIB.h
  SSLSocket.h
  StdAfx.cpp
  StdAfx.h
  TTProxy.cpp
  TTProxy.h
  TTProxy.rc
  TTX.def
  )

include_directories(
  ../teraterm/teraterm
  ../teraterm/common
  YCL/include
  ${OPENSSL_INCLUDE_DIRS}
  .
  )

add_library(
  ttproxy SHARED
  ${SRC}
  )

target_link_libraries(
  ttproxy
  ttpcmn
  ws2_32.lib
  )


