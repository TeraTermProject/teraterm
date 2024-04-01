﻿# cmake -P innosetup.cmake
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# check innosetup
#   6.2.2
set(CHECK_FILE innosetup6/bin/ISCC.exe)
set(CHECK_HASH "29856cf82d9a993afd7da8f99fb89f24d65ce3271c5b042cf387257c8fd7660f")
if(EXISTS ${CHECK_FILE})
  file(SHA256 ${CHECK_FILE} HASH)
  if(${HASH} STREQUAL ${CHECK_HASH})
    return()
  endif()
  message("file ${CHECK_FILE}")
  message("actual HASH=${HASH}")
  message("expect HASH=${CHECK_HASH}")
endif()

# unrar
#   https://www.rarlab.com/rar_add.htm
#   RAR/Extras page, UnRAR for Windows 7.00 x64
set(UNRAR_ZIP "unrarw64.exe")
set(UNRAR_URL "https://www.rarlab.com/rar/unrarw64.exe")
set(UNRAR_HASH "6e0ceb18906a4c66ba489d0a4aa35db4bd6aa454dff081459d427b872fdd3973")
file(MAKE_DIRECTORY "download/unrar")
file(DOWNLOAD
  ${UNRAR_URL}
  download/unrar/${UNRAR_ZIP}
  EXPECTED_HASH SHA256=${UNRAR_HASH}
  SHOW_PROGRESS
  )
if(EXISTS "unrar")
  file(REMOVE_RECURSE "unrar")
endif()
file(MAKE_DIRECTORY "unrar")
execute_process(
  COMMAND ../download/unrar/${UNRAR_ZIP} /s
  WORKING_DIRECTORY "unrar"
)
if(REMOVE_TMP)
  file(REMOVE_RECURSE "download/unrar")
endif()

# innounp
set(INNOUNP_RAR "innounp050.rar")
set(INNOUNP_URL "https://sourceforge.net/projects/innounp/files/innounp/innounp%200.50/${INNOUNP_RAR}/download")
set(INNOUNP_HASH "1d8837540ccc15d98245a1c73fd08f404b2a7bdfe7dc9bed2fdece818ff6df67")
message("download ${INNOUNP_RAR} from ${INNOUNP_URL}")
file(MAKE_DIRECTORY "download/innounp")
file(DOWNLOAD
  ${INNOUNP_URL}
  download/innounp/${INNOUNP_RAR}
  EXPECTED_HASH SHA256=${INNOUNP_HASH}
  SHOW_PROGRESS
  )
if(EXISTS "innounp")
    file(REMOVE_RECURSE "innounp")
endif()
file(MAKE_DIRECTORY "innounp")
execute_process(
  COMMAND ../unrar/UnRAR.exe x ../download/innounp/${INNOUNP_RAR}
  WORKING_DIRECTORY "innounp"
  )
if(REMOVE_TMP)
  file(REMOVE_RECURSE "download/innounp")
endif()

# innosetup 6
set(INNOSETUP_EXE "innosetup-6.2.2.exe")
set(INNOSETUP_URL "https://files.jrsoftware.org/is/6/${INNOSETUP_EXE}")
set(INNOSETUP_HASH "8117d10d00a2ad33a1390978ea3872861c330e087914410a6377b22c4c5b8563")
message("download ${INNOSETUP_EXE} from ${INNOSETUP_URL}")
file(MAKE_DIRECTORY "download/innosetup6")
file(DOWNLOAD
  ${INNOSETUP_URL}
  download/innosetup6/${INNOSETUP_EXE}
  EXPECTED_HASH SHA256=${INNOSETUP_HASH}
  SHOW_PROGRESS
  )
if(EXISTS "innosetup6")
  file(REMOVE_RECURSE "innosetup6")
endif()
file(MAKE_DIRECTORY "innosetup6")
execute_process(
  COMMAND ../innounp/innounp.exe -x ../download/innosetup6/${INNOSETUP_EXE}
  WORKING_DIRECTORY "innosetup6"
  )
file(RENAME "innosetup6/{app}" innosetup6/bin)
file(RENAME "innosetup6/{tmp}" innosetup6/tmp)
if(REMOVE_TMP)
  file(REMOVE_RECURSE "download/innosetup6")
endif()
