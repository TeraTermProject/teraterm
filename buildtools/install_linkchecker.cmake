# cmake -P install_linkchecker.cmake

# linkchecker
set(LINKCHECKER_PL "${CMAKE_CURRENT_LIST_DIR}/linkchecker/linkchecker.pl")
set(LINKCHECKER_URL "https://raw.githubusercontent.com/saoyagi2/linkchecker/8fa40149c382d0a1b54a06dc0a7f786fb945faf7/linkchecker.pl")
set(LINKCHECKER_HASH "a32923936265c7c9d9d91e0edda70e9b7f1157403aa0f4d5d4b391ed67257b0f")

# check linkchecker
if(EXISTS ${LINKCHECKER_PL})
  file(SHA256 ${LINKCHECKER_PL} HASH)
  if(${HASH} STREQUAL ${LINKCHECKER_HASH})
    return()
  endif()
  message("file ${LINKCHECKER_PL}")
  message("actual HASH=${HASH}")
  message("expect HASH=${LINKCHECKER_HASH}")
endif()

# download
message("download ${LINKCHECKER_PL} from ${LINKCHECKER_URL}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/linkchecker")
file(DOWNLOAD
  ${LINKCHECKER_URL}
  ${LINKCHECKER_PL}
  EXPECTED_HASH SHA256=${LINKCHECKER_HASH}
  SHOW_PROGRESS
  )
