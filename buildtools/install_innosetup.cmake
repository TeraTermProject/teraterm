# cmake -P innosetup.cmake
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# innosetup 7.1.1
set(INNOSETUP_EXE "innosetup-7.1.1-x64.exe")
set(INNOSETUP_URL "https://github.com/jrsoftware/issrc/releases/download/is-7_1_1/${INNOSETUP_EXE}")
set(INNOSETUP_HASH "8a5a6711c3766a6e5fb74f280ba0e3551e719ce94538d901fdbd1f8f9920b3f0")
set(INNOSETUP_CHECK_FILE innosetup7/ISCC.exe)
set(INNOSETUP_CHECK_HASH "e0f9f1dc32032650c6629f6dee41eb6d20ba140e4e3c07c25d8a24f9ba781644")

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
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/download/innosetup7")
file(DOWNLOAD
  ${INNOSETUP_URL}
  ${CMAKE_CURRENT_LIST_DIR}/download/innosetup7/${INNOSETUP_EXE}
  EXPECTED_HASH SHA256=${INNOSETUP_HASH}
  SHOW_PROGRESS
  )

# setup
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/innosetup7")
  file(REMOVE_RECURSE "${CMAKE_CURRENT_LIST_DIR}/innosetup7")
endif()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/innosetup7")
execute_process(
  COMMAND ${CMAKE_CURRENT_LIST_DIR}/download/innosetup7/${INNOSETUP_EXE} /dir=. /CURRENTUSER /PORTABLE=1 /VERYSILENT
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/innosetup7"
  )
if(REMOVE_TMP)
  file(REMOVE_RECURSE "${CMAKE_CURRENT_LIST_DIR}/download/innosetup7")
endif()
