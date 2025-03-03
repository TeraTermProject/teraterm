# cmake -P innosetup.cmake
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# innosetup 6.4.1
set(INNOSETUP_EXE "innosetup-6.4.1.exe")
set(INNOSETUP_URL "https://files.jrsoftware.org/is/6/${INNOSETUP_EXE}")
set(INNOSETUP_HASH "f41760e1f1ae15d2089bb6ab162e21720b92ae7506ed70667b39200063d68e34")
set(CHECK_FILE innosetup6/ISCC.exe)
set(CHECK_HASH "3a34c0beaf8c9c66774c1c1286a504db403e97e1b2fba691b3ba865dbce7edb2")

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
