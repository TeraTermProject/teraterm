# cmake -P install_linkchecker.cmake

# linkchecker
set(LINKCHECKER_PL "${CMAKE_CURRENT_LIST_DIR}/linkchecker/linkchecker.pl")
set(LINKCHECKER_URL "https://raw.githubusercontent.com/saoyagi2/linkchecker/8c268c1f2706a2113c730e1798777ca05ee10e0d/linkchecker.pl")
set(LINKCHECKER_HASH "4ec2aeb5384a09b8eb53c117b5f39e5d2661e8592da02b4d2de9ad43e04385a5")

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
