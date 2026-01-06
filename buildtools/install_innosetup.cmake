# cmake -P innosetup.cmake
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# innosetup 6.6.1
set(INNOSETUP_EXE "innosetup-6.6.1.exe")
set(INNOSETUP_URL "https://files.jrsoftware.org/is/6/${INNOSETUP_EXE}")
set(INNOSETUP_HASH "d243ce440c02705530699554fb9612b9b2bd7a2a90629cdb7f41e66f5faeb91f")
set(CHECK_FILE innosetup6/ISCC.exe)
set(CHECK_HASH "f8deb3e9e54303c178fd3513357ebc4d580a63bd072e24c221442b06f884b8fc")

# check innosetup
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${CHECK_FILE})
  file(SHA256 ${CMAKE_CURRENT_LIST_DIR}/${CHECK_FILE} HASH)
  if(${HASH} STREQUAL ${CHECK_HASH})
    return()
  endif()
  message("file ${CMAKE_CURRENT_LIST_DIR}/${CHECK_FILE}")
  message("actual HASH=${HASH}")
  message("expect HASH=${CHECK_HASH}")
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
