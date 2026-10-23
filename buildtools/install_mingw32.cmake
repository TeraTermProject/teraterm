# install MinGW (gcc) for older versions of Windows in this folder
#  cmake -P install_mingw32.cmake
#
# MinGW-Builds (niXman/mingw-builds-binaries)
#  - i686, thread model win32, msvcrt
#  - https://github.com/niXman/mingw-builds-binaries/releases

set(MINGW_VERSION "16.2.0")
set(MINGW_REV "rt_v14-rev1")
set(MINGW_ARCHIVE_NAME "i686-${MINGW_VERSION}-release-win32-dwarf-msvcrt-${MINGW_REV}.7z")
set(MINGW_URL "https://github.com/niXman/mingw-builds-binaries/releases/download/${MINGW_VERSION}-${MINGW_REV}/${MINGW_ARCHIVE_NAME}")
set(MINGW_ARCHIVE_HASH "067d23294bcc8a16624e9f7a9eb7f7d8ceb16a248aa7b9f0bfd928fb5a3cd6b4")
set(MINGW_DOWNLOAD_DIR "${CMAKE_CURRENT_LIST_DIR}/download/mingw32")
set(MINGW_ARCHIVE "${MINGW_DOWNLOAD_DIR}/${MINGW_ARCHIVE_NAME}")
set(MINGW_EXTRACT_DIR "${MINGW_DOWNLOAD_DIR}/extract")
set(MINGW_DIR "${CMAKE_CURRENT_LIST_DIR}/mingw32")
set(MINGW_BUILD_INFO "${MINGW_DIR}/build-info.txt")
set(MINGW_BUILD_INFO_HASH "1ad7fe6f5c8095264f258bde6e8c5b4e8d9e96beaf3f49207df63627556f4ab6")

# check installed
if(EXISTS ${MINGW_BUILD_INFO})
  file(SHA256 ${MINGW_BUILD_INFO} HASH)
  if(${HASH} STREQUAL ${MINGW_BUILD_INFO_HASH})
    message("${MINGW_DIR} is up to date (${MINGW_ARCHIVE_NAME})")
    return()
  endif()
  message("file ${MINGW_BUILD_INFO}")
  message("actual HASH=${HASH}")
  message("expect HASH=${MINGW_BUILD_INFO_HASH}")
endif()

# download
message("download ${MINGW_ARCHIVE} from ${MINGW_URL}")
file(MAKE_DIRECTORY ${MINGW_DOWNLOAD_DIR})
file(DOWNLOAD
  ${MINGW_URL}
  ${MINGW_ARCHIVE}
  EXPECTED_HASH SHA256=${MINGW_ARCHIVE_HASH}
  SHOW_PROGRESS
)

# setup
#  archive has mingw32/
if(EXISTS ${MINGW_DIR})
  file(REMOVE_RECURSE ${MINGW_DIR})
endif()
if(EXISTS ${MINGW_EXTRACT_DIR})
  file(REMOVE_RECURSE ${MINGW_EXTRACT_DIR})
endif()
message("extract ${MINGW_ARCHIVE}")
file(ARCHIVE_EXTRACT
  INPUT ${MINGW_ARCHIVE}
  DESTINATION ${MINGW_EXTRACT_DIR}
)
file(RENAME
  "${MINGW_EXTRACT_DIR}/mingw32"
  ${MINGW_DIR}
)
file(REMOVE_RECURSE ${MINGW_EXTRACT_DIR})
if(NOT EXISTS "${MINGW_DIR}/bin/i686-w64-mingw32-gcc.exe")
  message(FATAL_ERROR "not found ${MINGW_DIR}/bin/i686-w64-mingw32-gcc.exe")
endif()
message("installed ${MINGW_DIR}")
