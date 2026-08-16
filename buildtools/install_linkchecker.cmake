# cmake -P install_linkchecker.cmake

# linkchecker
set(LINKCHECKER_PL "${CMAKE_CURRENT_LIST_DIR}/linkchecker/linkchecker.pl")
set(LINKCHECKER_URL "https://raw.githubusercontent.com/saoyagi2/linkchecker/9820513624b4b2b32f0bad9b22bcf20215e6f5af/linkchecker.pl")
set(LINKCHECKER_HASH "6f1e4c178af2c11a59649c97c66811eb7dc32131909f42351ebfb53e10e65c79")

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
