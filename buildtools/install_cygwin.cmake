# install cygwin in this folder
#  cmake -P install_cygwin.cmake
#  cmake -DREMOVE_TMP=ON -P install_cygwin.cmake
set(CYGWIN_ROOT "${CMAKE_CURRENT_LIST_DIR}/cygwin64")
if(DEFINED ENV{REMOVE_TMP})
  set(REMOVE_TMP ON)
else()
  option(REMOVE_TMP "" OFF)
endif()

# cygroot
if(EXISTS "${CYGWIN_ROOT}")
  file(MAKE_DIRECTORY "${CYGWIN_ROOT}")
endif()

if("${CYGWIN_ROOT}" MATCHES "cygdrive")
  # cygwin の cmake を使用するとpath(CYGWIN_ROOT) が /cygdrive/c.. となり
  # setup.exe の --root オプションで処理できない
  message(FATAL_ERROR "check CMAKE_COMMAND (${CMAKE_COMMAND})")
endif()

##############################
# cygwin (64bit) latest

# setup-x86_64.exe 2.935
set(SETUP_URL "https://cygwin.com/setup-x86_64.exe")
set(SETUP_HASH_SHA256 "4175dfeb4c5c44d2a53596081f73eac962d648901199a841509b594cca831396")
set(SETUP "${CYGWIN_ROOT}/setup-x86_64.exe")

set(DOWNLOAD_SITE "http://mirrors.kernel.org/sourceware/cygwin/")
set(PACKAGE "${CMAKE_CURRENT_LIST_DIR}/download/cygwin_package")

# setup-x86_64.exe を準備
set(HASH "0")
if(EXISTS ${SETUP})
  file(SHA256 ${SETUP} HASH)
endif()
if(NOT (${HASH} STREQUAL ${SETUP_HASH_SHA256}))
  # DOWNLOAD
  file(DOWNLOAD
    ${SETUP_URL}
    ${PACKAGE}/setup-x86_64.exe
    EXPECTED_HASH SHA256=${SETUP_HASH_SHA256}
    SHOW_PROGRESS
  )
  file(COPY ${PACKAGE}/setup-x86_64.exe DESTINATION ${CYGWIN_ROOT})
endif()

# install packages
execute_process(
  COMMAND ${SETUP} --quiet-mode --wait --no-admin --root ${CYGWIN_ROOT} --site ${DOWNLOAD_SITE} --local-package-dir ${PACKAGE} --upgrade-also --packages cmake,bash,tar,make,perl,gcc-core,gcc-g++,icoutils
  WORKING_DIRECTORY ${CYGWIN_ROOT}
)
# install packages for notify
execute_process(
  COMMAND ${SETUP} --quiet-mode --wait --no-admin --root ${CYGWIN_ROOT} --site ${DOWNLOAD_SITE} --local-package-dir ${PACKAGE} --upgrade-also--packages perl-JSON,perl-LWP-Protocol-https
  WORKING_DIRECTORY ${CYGWIN_ROOT}
)

# remove archives
if(REMOVE_TMP)
  file(GLOB ARC_FILES "${PACKAGE}/http*")
  file(REMOVE_RECURSE ${ARC_FILES})
endif()

##############################
# cygwin 32bit from time machine
#  http://www.crouchingtigerhiddenfruitbat.org/Cygwin/timemachine.html

#set(SETUP_EXE setup-x86_64-2.909.exe)
#set(SETUP_HASH_SHA256 b9219acd1241ffa4d38e19587f1ccc2854f951e451f3858efc9d2e1fe19d375c)
#set(DOWNLOAD_SITE "http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/circa/64bit/2021/10/28/174906")
set(SETUP_EXE setup-x86_64-2.924.exe)
set(SETUP_URL "http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/setup/snapshots/${SETUP_EXE}")
set(SETUP_HASH_SHA256 edd0a64dc65087ffe453ca94b267169b39458a983b29ac31320fcaa983d0f97e)
set(SETUP "${CYGWIN_ROOT}/${SETUP_EXE}")

set(DOWNLOAD_SITE "http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/circa/64bit/2022/12/01/145510")
set(PACKAGE "${CMAKE_CURRENT_LIST_DIR}/download/cygwin32_package")

# setup-x86_64-2.924.exe を準備
set(HASH "0")
if(EXISTS ${SETUP})
  file(SHA256 ${SETUP} HASH)
endif()
if(NOT (${HASH} STREQUAL ${SETUP_HASH_SHA256}))
  # DWONLOAD
  file(DOWNLOAD
    ${SETUP_URL}
    ${PACKAGE}/${SETUP_EXE}
    EXPECTED_HASH SHA256=${SETUP_HASH_SHA256}
    SHOW_PROGRESS
  )
  file(COPY ${PACKAGE}/${SETUP_EXE} DESTINATION ${CYGWIN_ROOT})
endif()

# install packages
execute_process(
  COMMAND ${SETUP} -X --quiet-mode --wait --no-admin --root ${CYGWIN_ROOT} --site ${DOWNLOAD_SITE} --local-package-dir ${PACKAGE} --packages cygwin32-gcc-g++
  WORKING_DIRECTORY ${CYGWIN_ROOT}
)

# remove archives
if(REMOVE_TMP)
  file(GLOB ARC_FILES "${PACKAGE}/http*")
  file(REMOVE_RECURSE ${ARC_FILES})
endif()
