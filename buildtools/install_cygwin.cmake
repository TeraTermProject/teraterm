# install cygwin in this folder
#  cmake -P install_cygwin.cmake
set(CYGWIN_ROOT "${CMAKE_CURRENT_LIST_DIR}/cygwin64")

# cygroot
if(EXISTS "${CYGWIN_ROOT}")
  file(MAKE_DIRECTORY "${CYGWIN_ROOT}")
endif()

##############################
# cygwin (64bit) latest

set(SETUP_URL "https://cygwin.com/setup-x86_64.exe")
set(SETUP_HASH_SHA256 "edd0a64dc65087ffe453ca94b267169b39458a983b29ac31320fcaa983d0f97e")
set(SETUP "${CYGWIN_ROOT}/setup-x86_64.exe")

set(DOWNLOAD_SITE "http://mirrors.kernel.org/sourceware/cygwin/")
set(PACKAGE "${CMAKE_CURRENT_LIST_DIR}/download/cygwin_package")

# setup-x86_64.exe を準備
file(DOWNLOAD
  ${SETUP_URL}
  ${PACKAGE}/setup-x86_64.exe
  EXPECTED_HASH SHA256=${SETUP_HASH_SHA256}
  SHOW_PROGRESS
)
file(COPY ${PACKAGE}/setup-x86_64.exe DESTINATION ${CYGWIN_ROOT})

# install packages
execute_process(
  COMMAND ${SETUP} --quiet-mode --wait --no-admin --root ${CYGWIN_ROOT} --site ${DOWNLOAD_SITE} --local-package-dir ${PACKAGE} --packages bash,tar,make,perl,gcc-core,gcc-g++
  WORKING_DIRECTORY ${CYGWIN_ROOT}
)

##############################
# cygwin 32bit from time machine
#  http://www.crouchingtigerhiddenfruitbat.org/Cygwin/timemachine.html

set(SETUP_URL "http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/setup/snapshots/setup-x86_64-2.909.exe")
set(SETUP_HASH_SHA256 b9219acd1241ffa4d38e19587f1ccc2854f951e451f3858efc9d2e1fe19d375c)
set(SETUP "${CYGWIN_ROOT}/setup-x86_64-2.909.exe")

set(DOWNLOAD_SITE "http://ctm.crouchingtigerhiddenfruitbat.org/pub/cygwin/circa/64bit/2021/10/28/174906")
set(PACKAGE "${CMAKE_CURRENT_LIST_DIR}/download/cygwin32_package")

# setup-x86_64.exe を準備
file(DOWNLOAD
  ${SETUP_URL}
  ${PACKAGE}/setup-x86_64-2.909.exe
  EXPECTED_HASH SHA256=${SETUP_HASH_SHA256}
  SHOW_PROGRESS
)
file(COPY ${PACKAGE}/setup-x86_64-2.909.exe DESTINATION ${CYGWIN_ROOT})

# install packages
execute_process(
  COMMAND ${SETUP} -X --quiet-mode --wait --no-admin --root ${CYGWIN_ROOT} --site ${DOWNLOAD_SITE} --local-package-dir ${PACKAGE} --packages cygwin32-gcc-g++
  WORKING_DIRECTORY ${CYGWIN_ROOT}
)
