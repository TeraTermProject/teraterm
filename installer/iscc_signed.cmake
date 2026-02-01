# cmake -P iscc_signed.cmake

if(DEFINED ENV{arch})
  set(arch $ENV{arch})
else()
  set(arch "x86")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../buildtools/svnrev/build_config.cmake)

#file(REMOVE_RECURSE ${CMAKE_CURRENT_LIST_DIR}/Output/portable_signed/unzip)
#file(ARCHIVE_EXTRACT
#  INPUT ${CMAKE_CURRENT_LIST_DIR}/Output/portable/teraterm_portable_signed.zip
#  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/Output/portable_signed/unzip
#)

set(ISCC "${CMAKE_CURRENT_LIST_DIR}/../buildtools/innosetup6/ISCC.exe")

if(RELEASE)
  set(SETUP_EXE "teraterm-${VERSION}-${arch}")
else()
  set(SETUP_EXE "teraterm-${VERSION}-${arch}-${DATE}_${TIME}-${GITVERSION}-snapshot")
endif()
set(SRC_DIR "Output/portable_signed/teraterm-${arch}") # teraterm.iss相対
set(SETUP_DIR "${CMAKE_CURRENT_LIST_DIR}/Output/setup")
#set(SETUP_EXE "teraterm-unsigned")

set(INNO_SETUP_ARCH "x86")
if(arch STREQUAL "x64")
  set(INNO_SETUP_ARCH "x64")
elseif(arch STREQUAL "arm64")
  set(INNO_SETUP_ARCH "arm64")
endif()

file(MAKE_DIRECTORY ${SETUP_DIR})
execute_process(
  COMMAND ${ISCC} /DAppVersion=${VERSION} /DOutputBaseFilename=${SETUP_EXE} /DSrcDir=${SRC_DIR} /DArch=${INNO_SETUP_ARCH} /O${SETUP_DIR} ${CMAKE_CURRENT_LIST_DIR}/teraterm.iss
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/Output/portable_signed"
  ENCODING AUTO
  RESULT_VARIABLE rv
)
if(NOT rv STREQUAL "0")
  message(FATAL_ERROR "cmake build fail ${rv}")
endif()
