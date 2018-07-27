
#set(GENERATOR "Visual Studio 15 2017")
string(REPLACE " " "_" GENERATOR_ ${GENERATOR})

set(SRC_DIR_BASE "openssl-1.0.2o")
set(SRC_ARC "openssl-1.0.2o.tar.xz")
set(SRC_URL "https://www.openssl.org/source/openssl-1.0.2o.tar.gz")
set(SRC_ARC_HASH_SHA256 ec3f5c9714ba0fd45cb4e087301eb1336c317e0d20b575a125050470e8089e4d)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/donwload/openssl")

set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/openssl/src_${GENERATOR_}")
set(SRC_DIR "${EXTRACT_DIR}/${SRC_DIR_BASE}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/openssl_${GENERATOR_}")

########################################

file(DOWNLOAD
  ${SRC_URL}
  ${DOWN_DIR}/${SRC_ARC}
  EXPECTED_HASH SHA256=${SRC_ARC_HASH_SHA256}
  SHOW_PROGRESS
  )

########################################

find_program(
  PERL perl.exe
  HINTS c:/cygwin/usr/bin
  HINTS c:/cygwin64/usr/bin
  )
if(PERL-NOTFOUND)
  message(FATAL_ERROR "perl not found")
endif()
if(GENERATOR MATCHES "Visual Studio 15 2017")
  find_program(
	VCVARS32 vcvars32.bat
	HINTS "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build"
	HINTS "C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional/VC/Auxiliary/Build"
	HINTS "C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC/Auxiliary/Build"
	)
else()
  set(VCVARS32-NOTFOUND 1)
endif()  
if(VCVARS32-NOTFOUND)
  message(FATAL_ERROR "vcvars32.bat not found")
endif()

########################################

if(NOT EXISTS ${SRC_DIR}/README)

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTRACT_DIR}
    )

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xvf" ${DOWN_DIR}/${SRC_ARC}
    WORKING_DIRECTORY ${EXTRACT_DIR}
    )

endif()

########################################

file(TO_NATIVE_PATH ${INSTALL_DIR} INSTALL_DIR_N)
file(WRITE "${SRC_DIR}/build_cmake.bat"
  "call \"${VCVARS32}\"\n"
  "cd %~dp0\n"
  "${PERL} Configure no-asm VC-WIN32 --prefix=${INSTALL_DIR_N}\n"
  "call ms\\do_ms.bat\n"
  "nmake -f ms\\nt.mak install\n"
  )

set(BUILD_CMAKE_BAT "${SRC_DIR}/build_cmake.bat")
file(TO_NATIVE_PATH ${BUILD_CMAKE_BAT} BUILD_CMAKE_BAT_N)
execute_process(
  COMMAND cmd /c ${BUILD_CMAKE_BAT_N}
  WORKING_DIRECTORY ${SRC_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake build fail ${rv}")
endif()

########################################

set(EXTRACT_DIR "${EXTRACT_DIR}_debug")
set(SRC_DIR "${EXTRACT_DIR}/${SRC_DIR_BASE}")
set(INSTALL_DIR "${INSTALL_DIR}_debug")

########################################

if(NOT EXISTS ${SRC_DIR}/README)

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTRACT_DIR}
    )

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xvf" ${DOWN_DIR}/${SRC_ARC}
    WORKING_DIRECTORY ${EXTRACT_DIR}
    )

endif()

########################################

file(TO_NATIVE_PATH ${INSTALL_DIR} INSTALL_DIR_N)
file(WRITE "${SRC_DIR}/build_cmake.bat"
  "call \"${VCVARS32}\"\n"
  "cd %~dp0\n"
  "${PERL} Configure no-asm debug-VC-WIN32 --prefix=${INSTALL_DIR_N}\n"
  "call ms\\do_ms.bat\n"
  "nmake -f ms\\nt.mak install\n"
  )

set(BUILD_CMAKE_BAT "${SRC_DIR}/build_cmake.bat")
file(TO_NATIVE_PATH ${BUILD_CMAKE_BAT} BUILD_CMAKE_BAT_N)
execute_process(
  COMMAND cmd /c ${BUILD_CMAKE_BAT_N}
  WORKING_DIRECTORY ${SRC_DIR}
  RESULT_VARIABLE rv
  )
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake build fail ${rv}")
endif()

