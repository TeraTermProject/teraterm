# cmake -P install_nasm.cmake

# nasm 2.15.05
set(NASM_BASE "nasm-2.15.05")
set(NASM_ZIP "${NASM_BASE}-win32.zip")
set(NASM_URL "https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/win32/${NASM_ZIP}")
set(NASM_HASH "258c7d1076e435511cf2fdf94e2281eadbdb9e3003fd57f356f446e2bce3119e")
set(NASM_DOWNLOAD_DIR "${CMAKE_CURRENT_LIST_DIR}/download/nasm")
set(NASM_DIR "${CMAKE_CURRENT_LIST_DIR}/nasm")
set(NASM_CHECK_FILE "${NASM_DIR}/nasm.exe")
set(NASM_CHECK_HASH "a4735165141570d966b577d594c0541c29baed4978078c1c7237b855f8592a87")

# check nasm
if(EXISTS ${NASM_CHECK_FILE})
  file(SHA256 ${NASM_CHECK_FILE} HASH)
  if(${HASH} STREQUAL ${NASM_CHECK_HASH})
    return()
  endif()
  message("file ${NASM_CHECK_FILE}")
  message("actual HASH=${HASH}")
  message("expect HASH=${NASM_CHECK_HASH}")
endif()

# download
message("download ${NASM_ZIP} from ${NASM_URL}")
file(MAKE_DIRECTORY ${NASM_DOWNLOAD_DIR})
file(DOWNLOAD
  ${NASM_URL}
  ${NASM_DOWNLOAD_DIR}/${NASM_ZIP}
  EXPECTED_HASH SHA256=${NASM_HASH}
  SHOW_PROGRESS
  )

# setup
#  archive has nasm-2.15.05/
if(EXISTS ${NASM_DIR})
  file(REMOVE_RECURSE ${NASM_DIR})
endif()
if(EXISTS "${NASM_DOWNLOAD_DIR}/${NASM_BASE}")
  file(REMOVE_RECURSE "${NASM_DOWNLOAD_DIR}/${NASM_BASE}")
endif()
file(ARCHIVE_EXTRACT
  INPUT ${NASM_DOWNLOAD_DIR}/${NASM_ZIP}
  DESTINATION ${NASM_DOWNLOAD_DIR}
  )
file(RENAME "${NASM_DOWNLOAD_DIR}/${NASM_BASE}" ${NASM_DIR})
