set(PACKAGE_NAME "ttproxy")

project(${PACKAGE_NAME})

include(${CMAKE_CURRENT_SOURCE_DIR}/../libs/lib_openssl.cmake)

add_library(
  ${PACKAGE_NAME} SHARED
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

if(SUPPORT_OLD_WINDOWS)
  if(MSVC)
    target_sources(
      ${PACKAGE_NAME}
      PRIVATE
      ../../teraterm/common/compat_w95_vs2005.c
      )
  endif()
endif(SUPPORT_OLD_WINDOWS)

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  ../teraterm/common/ttlib.h
  ../teraterm/common/i18n.h
  ../teraterm/common/ttplugin.h
  ../teraterm/common/tttypes.h
  ../teraterm/common/tt-version.h
  ../teraterm/common/codeconv.h
  ../teraterm/common/dlglib.h
  ../teraterm/teraterm/ttdialog.h
  ../teraterm/teraterm/ttfileio.h
  ../teraterm/teraterm/ttsetup.h
  ../teraterm/teraterm/ttwsk.h
  )

source_group(
  "common"
  REGULAR_EXPRESSION
  "teraterm\/common\/")

target_sources(
  ${PACKAGE_NAME}
  PRIVATE
  YCL/include/YCL/Array.h
  YCL/include/YCL/ComboBoxCtrl.h
  YCL/include/YCL/common.h
  YCL/include/YCL/Dialog.h
  YCL/include/YCL/DynamicLinkLibrary.h
  YCL/include/YCL/EditBoxCtrl.h
  YCL/include/YCL/Enumeration.h
  YCL/include/YCL/FileVersion.h
  YCL/include/YCL/HASHCODE.h
  YCL/include/YCL/Hashtable.h
  YCL/include/YCL/IniFile.h
  YCL/include/YCL/Integer.h
  YCL/include/YCL/Object.h
  YCL/include/YCL/Pointer.h
  YCL/include/YCL/Resource.h
  YCL/include/YCL/String.h
  YCL/include/YCL/StringBuffer.h
  YCL/include/YCL/StringUtil.h
  YCL/include/YCL/ValueCtrl.h
  YCL/include/YCL/Vector.h
  YCL/include/YCL/Window.h
  )

source_group(
  "YCL"
  REGULAR_EXPRESSION
  "YCL\/include\/")

set_target_properties(
  ${PACKAGE_NAME}
  PROPERTIES
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/"
  )

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  ../teraterm/teraterm
  ../teraterm/common
  YCL/include
  ${OPENSSL_INCLUDE_DIRS}
  .
  )

target_link_libraries(
  ${PACKAGE_NAME}
  PRIVATE
  common_static
  ttpcmn
  #
  ws2_32
  version
  gdi32
  comdlg32
  )

install(
  TARGETS ${PACKAGE_NAME}
  RUNTIME
  DESTINATION .
  )
