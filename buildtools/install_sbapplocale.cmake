# cmake -P install_sbapplocale.cmake

# sbapplocale
set(SBAPPLOCALE_EXE "${CMAKE_CURRENT_LIST_DIR}/sbapplocale/sbapplocale.exe")
set(SBAPPLOCALE_EXE_HASH "bcabc2d5ab34bff8f147d12b69e0a019d16cf6c06859ba765e4e68f20c948577")
set(SBAPPLOCALE_URL "http://www.magicnotes.com/steelbytes/SBAppLocale_ENG.zip")
set(SBAPPLOCALE_ZIP "${CMAKE_CURRENT_LIST_DIR}/download/sbapplocale/SBAppLocale_ENG.zip")
set(SBAPPLOCALE_ZIP_HASH "e9edfd32cd5f03b36fac024f814ab8dd38e4880f3e6faacdea92d0e4d671fac1")

# check sbapplocale
if(EXISTS ${SBAPPLOCALE_EXE})
  file(SHA256 ${SBAPPLOCALE_EXE} HASH)
  if(${HASH} STREQUAL ${SBAPPLOCALE_EXE_HASH})
    return()
  endif()
  message("file ${SBAPPLOCALE_EXE}")
  message("actual HASH=${HASH}")
  message("expect HASH=${SBAPPLOCALE_EXE_HASH}")
endif()

# download
message("download ${SBAPPLOCALE_ZIP} from ${SBAPPLOCALE_URL}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/download/sbapplocale")
file(DOWNLOAD
  ${SBAPPLOCALE_URL}
  ${SBAPPLOCALE_ZIP}
  EXPECTED_HASH SHA256=${SBAPPLOCALE_ZIP_HASH}
  SHOW_PROGRESS
)

# setup
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sbapplocale")
  file(REMOVE_RECURSE "${CMAKE_CURRENT_LIST_DIR}/sbapplocale")
endif()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/sbapplocale")
file(ARCHIVE_EXTRACT
  INPUT ${SBAPPLOCALE_ZIP}
  DESTINATION "${CMAKE_CURRENT_LIST_DIR}/sbapplocale"
)
