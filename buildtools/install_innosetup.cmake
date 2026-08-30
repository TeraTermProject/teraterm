# cmake -P innosetup.cmake
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# innosetup 7.1.0
set(INNOSETUP_EXE "innosetup-7.1.0-x64.exe")
set(INNOSETUP_URL "https://github.com/jrsoftware/issrc/releases/download/is-7_1_0/${INNOSETUP_EXE}")
set(INNOSETUP_HASH "0362a383ed217d4c4239b5933866dd96d3eb2102737da92f80f6057a4b40df2f")
set(INNOSETUP_CHECK_FILE innosetup7/ISCC.exe)
set(INNOSETUP_CHECK_HASH "d06ebd38f38e3cee60a3c50cc45bd449d77e0bc6a5cabc607ea9886808e4de1a")

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
