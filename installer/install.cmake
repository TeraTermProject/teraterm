
message(CMAKE_HOST_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME})

# Inno Setup
#  Create setup.exe
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  find_program(
    ISCC ISCC.exe
    HINTS "${CMAKE_SOURCE_DIR}/../buildtools/innosetup6"
    HINTS "C:/Program Files (x86)/Inno Setup 6/"
    HINTS "C:/Program Files/Inno Setup 6/"
    HINTS "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
  )
  message("ISCC=${ISCC}")
endif()

message("CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")

include("${CMAKE_CURRENT_BINARY_DIR}/build_config.cmake")

if(NOT $ENV{USER})
  set(USER $ENV{USER})
elseif(NOT $ENV{USERNAME})
  set(USER $ENV{USERNAME})
else()
  set(USER "NONAME")
endif()

if (RELEASE)
  list(APPEND ISCC_OPTION "/DAppVersion=${VERSION}")
else()
  list(APPEND ISCC_OPTION "/DAppVersion=${VERSION}_${DATE}_${TIME}-${GITVERSION}")
endif()

if(DEFINED GITVERSION)
  message("git hash=\"${GITVERSION}\"")
  set(REVISION_TIME_USER "${DATE}_${TIME}-${GITVERSION}-${USER}")
elseif(DEFINED SVNVERSION)
  message("svn revision=\"${SVNVERSION}\"")
  set(REVISION_TIME_USER "r${SVNVERSION}-${DATE}_${TIME}-${USER}")
else()
  set(REVISION_TIME_USER "${DATE}_${TIME}-unknown-${USER}")
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/teraterm-${VERSION}-${ARCHITECTURE}-${REVISION_TIME_USER}")
message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX})

if(NOT DEFINED SETUP_EXE)
  # not need ".exe"
  if(RELEASE)
    set(SETUP_EXE "teraterm-${VERSION}-${ARCHITECTURE}")
  else()
    set(SETUP_EXE "teraterm-${VERSION}-${ARCHITECTURE}-${REVISION_TIME_USER}")
  endif()
endif()
if(NOT DEFINED SETUP_ZIP)
  if(RELEASE)
    set(SETUP_ZIP "teraterm-${VERSION}-${ARCHITECTURE}.zip")
  else()
    set(SETUP_ZIP "teraterm-${VERSION}-${ARCHITECTURE}-${REVISION_TIME_USER}.zip")
  endif()
endif()

# Inno Setup
#  Create setup.exe
if(ISCC)
  if("${ARCHITECTURE}" STREQUAL "arm64")
    list(APPEND ISCC_OPTION "/DArch=arm64")
  elseif(("${ARCHITECTURE}" STREQUAL "x64") OR ("${ARCHITECTURE}" STREQUAL "x86_64"))
    list(APPEND ISCC_OPTION "/DArch=x64")
  elseif(("${ARCHITECTURE}" STREQUAL "win32") OR ("${ARCHITECTURE}" STREQUAL "i686"))
    list(APPEND ISCC_OPTION "/DArch=x86")
  else()
    message(FATAL_ERROR "ARCHITECTURE=${ARCHITECTURE}")
  endif()
  file(TOUCH ${CMAKE_INSTALL_PREFIX}/cygterm+.tar.gz)
  execute_process(
    COMMAND ${ISCC} ${ISCC_OPTION} /O${CMAKE_BINARY_DIR} /DOutputBaseFilename=${SETUP_EXE} /DSrcDir=${CMAKE_INSTALL_PREFIX} ${CMAKE_CURRENT_LIST_DIR}/teraterm.iss
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  file(SHA256 ${SETUP_EXE}.exe SHA_OUT)
  file(WRITE "${CMAKE_BINARY_DIR}/${SETUP_EXE}.exe.sha256sum" "${SHA_OUT} ${SETUP_EXE}.exe")
  file(SHA512 ${SETUP_EXE}.exe SHA_OUT)
  file(WRITE "${CMAKE_BINARY_DIR}/${SETUP_EXE}.exe.sha512sum" "${SHA_OUT} ${SETUP_EXE}.exe")
endif(ISCC)

# zip
file(RENAME ${CMAKE_INSTALL_PREFIX}/TTXFixedWinSize.dll ${CMAKE_INSTALL_PREFIX}/_TTXFixedWinSize.dll)
file(RENAME ${CMAKE_INSTALL_PREFIX}/TTXOutputBuffering.dll ${CMAKE_INSTALL_PREFIX}/_TTXOutputBuffering.dll)
file(RENAME ${CMAKE_INSTALL_PREFIX}/TTXResizeWin.dll ${CMAKE_INSTALL_PREFIX}/_TTXResizeWin.dll)
file(RENAME ${CMAKE_INSTALL_PREFIX}/TTXShowCommandLine.dll ${CMAKE_INSTALL_PREFIX}/_TTXShowCommandLine.dll)
file(RENAME ${CMAKE_INSTALL_PREFIX}/TTXtest.dll ${CMAKE_INSTALL_PREFIX}/_TTXtest.dll)
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E tar cvf ${SETUP_ZIP} --format=zip ${CMAKE_INSTALL_PREFIX}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
file(RENAME ${CMAKE_INSTALL_PREFIX}/_TTXFixedWinSize.dll ${CMAKE_INSTALL_PREFIX}/TTXFixedWinSize.dll)
file(RENAME ${CMAKE_INSTALL_PREFIX}/_TTXOutputBuffering.dll ${CMAKE_INSTALL_PREFIX}/TTXOutputBuffering.dll)
file(RENAME ${CMAKE_INSTALL_PREFIX}/_TTXResizeWin.dll ${CMAKE_INSTALL_PREFIX}/TTXResizeWin.dll)
file(RENAME ${CMAKE_INSTALL_PREFIX}/_TTXShowCommandLine.dll ${CMAKE_INSTALL_PREFIX}/TTXShowCommandLine.dll)
file(RENAME ${CMAKE_INSTALL_PREFIX}/_TTXtest.dll ${CMAKE_INSTALL_PREFIX}/TTXtest.dll)

file(SHA256 ${SETUP_EXE}.zip SHA_OUT)
file(WRITE "${CMAKE_BINARY_DIR}/${SETUP_EXE}.zip.sha256sum" "${SHA_OUT} ${SETUP_EXE}.zip")
file(SHA512 ${SETUP_EXE}.zip SHA_OUT)
file(WRITE "${CMAKE_BINARY_DIR}/${SETUP_EXE}.zip.sha512sum" "${SHA_OUT} ${SETUP_EXE}.zip")
