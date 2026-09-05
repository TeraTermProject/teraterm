# cmake -P install_lychee.cmake

# lychee
set(LYCHEE_VERSION "0.24.2")
set(LYCHEE_EXE "${CMAKE_CURRENT_LIST_DIR}/lychee/lychee.exe")
set(LYCHEE_EXE_HASH "2d15a3f78ac680103720b0c59667dc6f1da7de35f7e9c95171bdf4b76fb6829b")
set(LYCHEE_URL "https://github.com/lycheeverse/lychee/releases/download/lychee-v${LYCHEE_VERSION}/lychee-x86_64-pc-windows-msvc.zip")
set(LYCHEE_ZIP "${CMAKE_CURRENT_LIST_DIR}/download/lychee/lychee-x86_64-pc-windows-msvc.zip")
set(LYCHEE_ZIP_HASH "32975d1493ee1a975d6bb41e4fb56fe419cb442ded628bb772ba2e614acfacad")

# check lychee
if(EXISTS ${LYCHEE_EXE})
  file(SHA256 ${LYCHEE_EXE} HASH)
  if(${HASH} STREQUAL ${LYCHEE_EXE_HASH})
    return()
  endif()
  message("file ${LYCHEE_EXE}")
  message("actual HASH=${HASH}")
  message("expect HASH=${LYCHEE_EXE_HASH}")
endif()

# download
message("download ${LYCHEE_ZIP} from ${LYCHEE_URL}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/download/lychee")
file(DOWNLOAD
  ${LYCHEE_URL}
  ${LYCHEE_ZIP}
  EXPECTED_HASH SHA256=${LYCHEE_ZIP_HASH}
  SHOW_PROGRESS
)

# setup
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/lychee")
  file(REMOVE_RECURSE "${CMAKE_CURRENT_LIST_DIR}/lychee")
endif()
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/download/lychee/lychee-x86_64-pc-windows-msvc")
  file(REMOVE_RECURSE "${CMAKE_CURRENT_LIST_DIR}/download/lychee/lychee-x86_64-pc-windows-msvc")
endif()
file(ARCHIVE_EXTRACT
  INPUT ${LYCHEE_ZIP}
  DESTINATION "${CMAKE_CURRENT_LIST_DIR}/download/lychee"
)
file(RENAME
  "${CMAKE_CURRENT_LIST_DIR}/download/lychee/lychee-x86_64-pc-windows-msvc"
  "${CMAKE_CURRENT_LIST_DIR}/lychee"
)
