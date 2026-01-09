# cmake -P innosetup.cmake
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# innosetup 6.7.0
set(INNOSETUP_EXE "innosetup-6.7.0.exe")
set(INNOSETUP_URL "https://files.jrsoftware.org/is/6/${INNOSETUP_EXE}")
set(INNOSETUP_HASH "f45c7d68d1e660cf13877ec36738a5179ce72a33414f9959d35e99b68c52a697")
set(INNOSETUP_CHECK_FILE innosetup6/ISCC.exe)
set(INNOSETUP_CHECK_HASH "a7472a35be9cd63da59e54c2622a0d8b048ab2ec85382e2b2d47a5a79dbf9bef")

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
