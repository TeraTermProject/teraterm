project(ttproxy)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

include(${CMAKE_CURRENT_SOURCE_DIR}/../libs/lib_openssl.cmake)

set(COMMON_SRC
  ../teraterm/common/ttlib.h
  ../teraterm/common/i18n.h
  ../teraterm/teraterm/ttdialog.h
  ../teraterm/teraterm/ttfileio.h
  ../teraterm/common/ttplugin.h
  ../teraterm/teraterm/ttsetup.h
  ../teraterm/common/tttypes.h
  ../teraterm/common/tt-version.h
  ../teraterm/teraterm/ttwsk.h
  )
source_group(
  "common"
  FILES
  ${COMMON_SRC}
  )

set(YCL_SRC
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
  FILES
  ${YCL_SRC}
  )

set(SRC
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
  )

include_directories(
  ../teraterm/teraterm
  ../teraterm/common
  YCL/include
  ${OPENSSL_INCLUDE_DIRS}
  .
  )

if(MINGW)
  link_directories(
	${CMAKE_SOURCE_DIR}/teraterm/htmlhelp
	)
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-uTTXBind@8")

add_library(
  ttproxy SHARED
  ${SRC}
  ${COMMON_SRC}
  ${YCL_SRC}
  )

set_target_properties(
  ttproxy
  PROPERTIES
  PREFIX ""
  OUTPUT_NAME "TTXProxy"
  )

target_link_libraries(
  ttproxy
  ttpcmn
  #
  ws2_32
  version
  gdi32
  comdlg32
  )


