# cmake -P innosetup.cmake
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# innosetup 6.7.2
set(INNOSETUP_EXE "innosetup-6.7.2.exe")
set(INNOSETUP_URL "https://github.com/jrsoftware/issrc/releases/download/is-6_7_2/${INNOSETUP_EXE}")
set(INNOSETUP_HASH "9f27f8386e554eb093336a1ca5c2dcacb7dbf04ab020889491d7ac53c38a12ff")
set(INNOSETUP_CHECK_FILE innosetup6/ISCC.exe)
set(INNOSETUP_CHECK_HASH "bb8d5f6cfaddc1446d1db2b900b056b8393faaeaf987bdfe25a2905702d2c9c6")

# check innosetup
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${INNOSETUP_CHECK_FILE})
  file(SHA256 ${CMAKE_CURRENT_LIST_DIR}/${INNOSETUP_CHECK_FILE} HASH)
  if(${HASH} STREQUAL ${INNOSETUP_CHECK_HASH})
    return()
  endif()
  message("file ${CMAKE_CURRENT_LIST_DIR}/${INNOSETUP_CHECK_FILE}")
  message("actual HASH=${HASH}")
  message("expect HASH=${INNOSETUP_CHECK_HASH}")
endif()

# download
message("download ${INNOSETUP_EXE} from ${INNOSETUP_URL}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/download/innosetup6")
file(DOWNLOAD
  ${INNOSETUP_URL}
  ${CMAKE_CURRENT_LIST_DIR}/download/innosetup6/${INNOSETUP_EXE}
  EXPECTED_HASH SHA256=${INNOSETUP_HASH}
  SHOW_PROGRESS
  )

# setup
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/innosetup6")
  file(REMOVE_RECURSE "${CMAKE_CURRENT_LIST_DIR}/innosetup6")
endif()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/innosetup6")
execute_process(
  COMMAND ${CMAKE_CURRENT_LIST_DIR}/download/innosetup6/${INNOSETUP_EXE} /dir=. /CURRENTUSER /PORTABLE=1 /VERYSILENT
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/innosetup6"
  )
if(REMOVE_TMP)
  file(REMOVE_RECURSE "${CMAKE_CURRENT_LIST_DIR}/download/innosetup6")
endif()
