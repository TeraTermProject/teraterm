# cmake -P innosetup.cmake
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# innosetup 6.3.0
set(INNOSETUP_EXE "innosetup-6.3.0.exe")
set(INNOSETUP_URL "https://files.jrsoftware.org/is/6/${INNOSETUP_EXE}")
set(INNOSETUP_HASH "caaaaf91d43b2878bbb63ae90aa2f0e144b5ec85a75f19e423e4fc4ddc52e157")
set(CHECK_FILE innosetup6/ISCC.exe)
set(CHECK_HASH "5b38e41bee7f8321ad0c14a53a15368f0e433aff190c1437caca7ec41787d3bc")

# check innosetup
if(EXISTS ${CHECK_FILE})
  file(SHA256 ${CHECK_FILE} HASH)
  if(${HASH} STREQUAL ${CHECK_HASH})
    return()
  endif()
  message("file ${CHECK_FILE}")
  message("actual HASH=${HASH}")
  message("expect HASH=${CHECK_HASH}")
endif()

# download
message("download ${INNOSETUP_EXE} from ${INNOSETUP_URL}")
file(MAKE_DIRECTORY "download/innosetup6")
file(DOWNLOAD
  ${INNOSETUP_URL}
  download/innosetup6/${INNOSETUP_EXE}
  EXPECTED_HASH SHA256=${INNOSETUP_HASH}
  SHOW_PROGRESS
  )

# setup
if(EXISTS "innosetup6")
  file(REMOVE_RECURSE "innosetup6")
endif()
file(MAKE_DIRECTORY "innosetup6")
execute_process(
  COMMAND ../download/innosetup6/${INNOSETUP_EXE} /dir=. /CURRENTUSER /PORTABLE=1 /VERYSILENT
  WORKING_DIRECTORY "innosetup6"
  )
if(REMOVE_TMP)
  file(REMOVE_RECURSE "download/innosetup6")
endif()
