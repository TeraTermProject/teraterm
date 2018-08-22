# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P openssl.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -DCMAKE_CONFIGURATION_TYPE=Release -P openssl.cmake

####
if(("${CMAKE_BUILD_TYPE}" STREQUAL "") AND ("${CMAKE_CONFIGURATION_TYPE}" STREQUAL ""))
  if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
	# multi-configuration
	execute_process(
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
	  -DCMAKE_CONFIGURATION_TYPE=Release
	  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
	  -P openssl.cmake
	  )
	execute_process(
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
	  -DCMAKE_CONFIGURATION_TYPE=Debug
	  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
	  -P openssl.cmake
	  )
	return()
  elseif("${CMAKE_GENERATOR}" MATCHES "Unix Makefiles")
	# mingw
	# single-configuration
	if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
	  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../mingw.toolchain.cmake")
	endif()
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  elseif("${CMAKE_GENERATOR}" MATCHES "NMake Makefiles")
	# VS nmake
	# single-configuration
	if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
	  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/VSToolchain.cmake")
	endif()
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  else()
	# single-configuration
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  endif()
endif()

include(script_support.cmake)

set(SRC_DIR_BASE "openssl-1.0.2p")
set(SRC_ARC "openssl-1.0.2p.tar.gz")
set(SRC_URL "https://www.openssl.org/source/openssl-1.0.2p.tar.gz")
set(SRC_ARC_HASH_SHA256 50a98e07b1a89eb8f6a99477f262df71c6fa7bef77df4dc83025a2845c827d00)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/openssl")

set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/openssl/src_${TOOLSET}")
set(SRC_DIR "${EXTRACT_DIR}/${SRC_DIR_BASE}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/openssl_${TOOLSET}")

if((${CMAKE_GENERATOR} MATCHES "Visual Studio") AND ("${CMAKE_CONFIGURATION_TYPE}" STREQUAL "Debug"))
  set(EXTRACT_DIR "${EXTRACT_DIR}_debug")
  set(SRC_DIR "${EXTRACT_DIR}/${SRC_DIR_BASE}")
  set(INSTALL_DIR "${INSTALL_DIR}_debug")
endif()

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
  HINTS c:/Perl64/bin
  HINTS c:/Perl/bin
  HINTS c:/cygwin/usr/bin
  HINTS c:/cygwin64/usr/bin
  )
if(PERL-NOTFOUND)
  message(FATAL_ERROR "perl not found")
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

if((${CMAKE_GENERATOR} MATCHES "Visual Studio") OR
	(${CMAKE_GENERATOR} MATCHES "NMake Makefiles"))
  ######################################## VS
  if(${CMAKE_GENERATOR} MATCHES "NMake Makefiles")
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 15 2017")
	find_program(
	  VCVARS32 vcvars32.bat
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build"
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional/VC/Auxiliary/Build"
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC/Auxiliary/Build"
	  )
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 14 2015")
	find_program(
	  VCVARS32 vcvars32.bat
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin"
	  )
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 12 2013")
	find_program(
	  VCVARS32 vcvars32.bat
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/bin"
	  )
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 11 2012")
	find_program(
	  VCVARS32 vcvars32.bat
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin"
	  )
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 10 2010")
	find_program(
	  VCVARS32 vcvars32.bat
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin"
	  )
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 9 2008")
	find_program(
	  VCVARS32 vcvars32.bat
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio 9.0/VC/bin"
	  )
  elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005")
	find_program(
	  VCVARS32 vcvars32.bat
	  HINTS "C:/Program Files (x86)/Microsoft Visual Studio 8/VC/bin"
	  )
  else()
	message(FATAL_ERROR "CMAKE_GENERATOR ${CMAKE_GENERATOR} not supported")
  endif()  
  if(VCVARS32-NOTFOUND)
	message(FATAL_ERROR "vcvars32.bat not found")
  endif()

  if(("${CMAKE_BUILD_TYPE}" STREQUAL "Release") OR ("${CMAKE_CONFIGURATION_TYPE}" STREQUAL "Release"))
	set(CONFIG_TARGET "VC-WIN32")
  else()
	set(CONFIG_TARGET "debug-VC-WIN32")
  endif()

  file(WRITE "${SRC_DIR}/build_cmake.bat"
	"cd %~dp0\n"
	)
  file(TO_NATIVE_PATH ${PERL} PERL_N)
  file(TO_NATIVE_PATH ${INSTALL_DIR} INSTALL_DIR_N)
  string(REGEX REPLACE [[^(.*)\\.*$]] [[\1]] PERL_N_PATH ${PERL_N})
  file(APPEND "${SRC_DIR}/build_cmake.bat"
	"del crypto\\buildinf.h\n"
	"set PATH=${PERL_N_PATH}\n"
	)
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
	file(TO_NATIVE_PATH ${VCVARS32} VCVARS32_N)
	file(APPEND "${SRC_DIR}/build_cmake.bat"
	  "call \"${VCVARS32_N}\"\n"
	  )
  endif()
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005")
	## Visual Studio 2005 特別処理
	# include,libパスの設定
	file(APPEND "${SRC_DIR}/build_cmake.bat"
	  "set SDK=C:\\Program Files\\Microsoft SDKs\\Windows\\v7.1\n"
	  "set INCLUDE=%SDK%\\Include;%INCLUDE%\n"
	  "set LIB=%SDK%\\lib;%LIB%\n"
	  )
  endif()
  file(APPEND "${SRC_DIR}/build_cmake.bat"
	"perl Configure no-asm ${CONFIG_TARGET} --prefix=${INSTALL_DIR_N}\n"
	"call ms\\do_ms.bat\n"
	"nmake -f ms\\nt.mak install ${NMAKE_OPTION}\n"
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
else()
  ######################################## MinGW
  find_program(
	MAKE make.exe
	HINTS c:/cygwin/usr/bin
	HINTS c:/cygwin64/usr/bin
	)
  include(${CMAKE_SOURCE_DIR}/../mingw.toolchain.cmake)
  set(ENV{PREFIX} i686-w64-mingw32)
  set(ENV{CC} ${CMAKE_C_COMPILER})
  set(ENV{CXX} ${CMAKE_CXX_COMPILER})
  set(ENV{AR} "i686-w64-mingw32-ar")
  set(ENV{RANLIB} "i686-w64-mingw32-ranlib")
  execute_process(
	COMMAND ${PERL} ./Configure mingw --prefix=${INSTALL_DIR}
	WORKING_DIRECTORY ${SRC_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake configure fail ${rv}")
  endif()
  execute_process(
	COMMAND ${MAKE}
	WORKING_DIRECTORY ${SRC_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake build fail ${rv}")
  endif()
  execute_process(
	COMMAND ${MAKE} install
	WORKING_DIRECTORY ${SRC_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake install fail ${rv}")
  endif()
endif()

